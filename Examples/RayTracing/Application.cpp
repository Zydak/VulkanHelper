#include "Application.h"
#include "Core/Error.h"

static std::tuple<VulkanHelper::Image, VulkanHelper::ImageView, VulkanHelper::Image, VulkanHelper::ImageView> CreateImages(
    VulkanHelper::Device& device,
    VulkanHelper::Window& window,
    VulkanHelper::CommandBuffer& commandBuffer,
    VulkanHelper::Format format
)
{
    VulkanHelper::Image::Config colorImageConfig{};
    colorImageConfig.Device = &device;
    colorImageConfig.Width = window.GetWidth();
    colorImageConfig.Height = window.GetHeight();
    colorImageConfig.Format = format;
    colorImageConfig.Usage = VulkanHelper::Image::Usage::STORAGE_BIT | VulkanHelper::Image::Usage::TRANSFER_SRC_BIT;
    colorImageConfig.Tiling = VulkanHelper::Image::Tiling::OPTIMAL;
    colorImageConfig.Aspect = VulkanHelper::Image::Aspect::COLOR_BIT;
    colorImageConfig.SampleCount = VulkanHelper::SampleCount::COUNT_1_BIT;
    auto outColorImage = VulkanHelper::Image::New(colorImageConfig).Value();
    outColorImage.TransitionImageLayout(
        VulkanHelper::Image::Layout::GENERAL,
        commandBuffer
    );

    auto outColorImageView = VulkanHelper::ImageView::New({
        &outColorImage,
        VulkanHelper::ImageView::ViewType::VIEW_2D
    }).Value();

    VulkanHelper::Image::Config depthImageConfig{};
    depthImageConfig.Device = &device;
    depthImageConfig.Width = window.GetWidth();
    depthImageConfig.Height = window.GetHeight();
    depthImageConfig.Format = VulkanHelper::Format::D32_SFLOAT;
    depthImageConfig.Usage = VulkanHelper::Image::Usage::DEPTH_STENCIL_ATTACHMENT_BIT;
    depthImageConfig.Tiling = VulkanHelper::Image::Tiling::OPTIMAL;
    depthImageConfig.Aspect = VulkanHelper::Image::Aspect::DEPTH_BIT;
    depthImageConfig.SampleCount = VulkanHelper::SampleCount::COUNT_1_BIT;
    auto outDepthImage = VulkanHelper::Image::New(depthImageConfig).Value();

    auto outDepthImageView = VulkanHelper::ImageView::New({
        &outDepthImage,
        VulkanHelper::ImageView::ViewType::VIEW_2D
    }).Value();

    return std::make_tuple(std::move(outColorImage), std::move(outColorImageView), std::move(outDepthImage), std::move(outDepthImageView));
}

Application Application::New()
{
    VulkanHelper::ThreadPool threadPool(4);

    VulkanHelper::AssetImporter importer = VulkanHelper::AssetImporter::New({&threadPool}).Value();

    auto scene = importer.ImportScene("../../../ModelViewer/Assets/VikingRoom.gltf").get();
    VH_ASSERT(scene.HasValue(), "Failed to import scene");

    VH_LOG_INFO("Current working directory: {}", std::filesystem::current_path().string());
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({true}).Value();
    VulkanHelper::Vector<const char*> extensions;
    auto physicalDevices = instance.GetSuitablePhysicalDevices();
    VH_ASSERT(!physicalDevices.Empty(), "No suitable physical devices found!");

    // Prefer a discrete GPU if available
    VulkanHelper::PhysicalDevice* selectedDevice = &physicalDevices[0];
    for (size_t i = 0; i < physicalDevices.Size(); i++)
    {   
        if (physicalDevices[i].IsDiscrete())
        {
            VH_LOG_INFO("Selected Discrete GPU: {}", physicalDevices[i].GetName());
            selectedDevice = &physicalDevices[i];
            break;
        }
    }

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 900, 900, "Example Project", "", true}).Value();
    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, {&window}, &instance}).Value();
    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window}).Value();

    VulkanHelper::Shader::InitializeSession("../../../RayTracing/Shaders/");

    VulkanHelper::Shader rgenShader = VulkanHelper::Shader::New({&device, "RayGen.slang", VulkanHelper::ShaderStages::RAYGEN_BIT}).Value();
    VulkanHelper::Shader hitShader = VulkanHelper::Shader::New({&device, "ClosestHit.slang", VulkanHelper::ShaderStages::CLOSEST_HIT_BIT}).Value();
    VulkanHelper::Shader missShader = VulkanHelper::Shader::New({&device, "Miss.slang", VulkanHelper::ShaderStages::MISS_BIT}).Value();

    VulkanHelper::CommandPool commandPool = VulkanHelper::CommandPool::New({&device, VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, device.GetQueueFamilyIndices().GraphicsFamily}).Value();
    VulkanHelper::CommandBuffer initializationCmd = commandPool.AllocateCommandBuffer({}).Value();

    VH_ASSERT(initializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Failed to begin recording initialization command buffer");
    auto [colorImage, colorImageView, depthImage, depthImageView] = CreateImages(device, window, initializationCmd, renderer.GetSwapchainImageFormat());

    std::array<VulkanHelper::Format, 3> vertexAttributes = {
        VulkanHelper::Format::R32G32B32_SFLOAT, // Position
        VulkanHelper::Format::R32G32B32_SFLOAT, // Normal
        VulkanHelper::Format::R32G32_SFLOAT, // UV
    };
    
    VulkanHelper::Mesh::Config meshConfig{};
    meshConfig.Device = &device;
    meshConfig.VertexAttributes = vertexAttributes.data();
    meshConfig.VertexAttributeCount = vertexAttributes.size();
    meshConfig.VertexData = scene.Value().Meshes[0].Vertices.Data();
    meshConfig.VertexDataSize = scene.Value().Meshes[0].Vertices.Size() * sizeof(VulkanHelper::Vertex);
    meshConfig.IndexData = scene.Value().Meshes[0].Indices.Data();
    meshConfig.IndexDataSize = scene.Value().Meshes[0].Indices.Size() * sizeof(uint32_t);
    meshConfig.CommandBuffer = &initializationCmd;

    VulkanHelper::Mesh loadedMesh = VulkanHelper::Mesh::New(meshConfig).Value();

    VulkanHelper::PushConstant pushConstant = VulkanHelper::PushConstant::New({
        VulkanHelper::ShaderStages::VERTEX_BIT,
        nullptr, // No initial data
        sizeof(glm::mat4)
    }).Value();

    // Descriptor set
    std::array<VulkanHelper::DescriptorPool::PoolSize, 1> poolSizes = {
        VulkanHelper::DescriptorPool::PoolSize{VulkanHelper::DescriptorType::STORAGE_IMAGE, 1},
    };
    
    VulkanHelper::DescriptorPool::Config descriptorPoolConfig{};
    descriptorPoolConfig.Device = &device;
    descriptorPoolConfig.MaxSets = 1;
    descriptorPoolConfig.PoolSizes = poolSizes.data();
    descriptorPoolConfig.PoolSizeCount = poolSizes.size();
    VulkanHelper::DescriptorPool descriptorPool = VulkanHelper::DescriptorPool::New(descriptorPoolConfig).Value();

    std::array<VulkanHelper::DescriptorSet::BindingDescription, 1> bindingDescriptions = {
        VulkanHelper::DescriptorSet::BindingDescription{0, 1, VulkanHelper::ShaderStages::RAYGEN_BIT, VulkanHelper::DescriptorType::STORAGE_IMAGE},
    };
    VulkanHelper::DescriptorSet::Config descriptorSetConfig{};
    descriptorSetConfig.Bindings = bindingDescriptions.data();
    descriptorSetConfig.BindingCount = bindingDescriptions.size();

    VulkanHelper::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSet(descriptorSetConfig).Value();
    VH_ASSERT(descriptorSet.AddImage(0, 0, colorImageView, VulkanHelper::Image::Layout::GENERAL) == VulkanHelper::VHResult::OK, "Failed to add image to descriptor set");

    VulkanHelper::Pipeline::RayTracingConfig pipelineConfig{};
    pipelineConfig.Device = &device;
    pipelineConfig.RayGenShaders.PushBack(&rgenShader);
    pipelineConfig.HitShaders.PushBack(&hitShader);
    pipelineConfig.MissShaders.PushBack(&missShader);
    pipelineConfig.PushConstant = &pushConstant;
    pipelineConfig.DescriptorSets.PushBack(&descriptorSet);
    pipelineConfig.CommandBuffer = &initializationCmd;

    VulkanHelper::Pipeline rtPipeline = VulkanHelper::Pipeline::New(pipelineConfig).Value();

    VH_ASSERT(initializationCmd.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end recording initialization command buffer");
    VH_ASSERT(initializationCmd.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit initialization command buffer");

    return Application(
        std::move(instance),
        std::move(window),
        std::move(device),
        std::move(renderer),
        std::move(rgenShader),
        std::move(hitShader),
        std::move(missShader),
        std::move(rtPipeline),
        std::move(loadedMesh),
        std::move(descriptorPool),
        std::move(descriptorSet),
        std::move(depthImage),
        std::move(depthImageView),
        std::move(colorImage),
        std::move(colorImageView),
        std::move(pushConstant),
        std::move(commandPool),
        std::move(initializationCmd)
    );
}

void Application::Run()
{
    glm::mat4 projection = glm::perspective(glm::radians(38.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for model transformation
    model = glm::translate(model, glm::vec3(0.0f, 0.25f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Blender has some really weird coordinate system
    auto timer = std::chrono::high_resolution_clock::now();
    while (!m_Window.WantsToClose())
    {
        {
            float time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - timer).count();

            glm::vec3 position = glm::vec3(glm::sin(time / 2.0f) * 3.0f, -1.5f, glm::cos(time / 2.0f) * 3.0f);
            glm::mat4 view = glm::lookAt(
                position,
                glm::vec3(0.0f, 0.0f, 0.0f), // Look at point
                glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
            );
            glm::mat4 mvp = projection * view * model;

            VH_ASSERT(m_PushConstant.SetData(&mvp, sizeof(mvp)) == VulkanHelper::VHResult::OK, "Failed to set push constant data");
        }

        VulkanHelper::Window::PollEvents();
        static glm::uvec2 lastWindowSize = {m_Window.GetWidth(), m_Window.GetHeight()};
        if (lastWindowSize.x != m_Window.GetWidth() || lastWindowSize.y != m_Window.GetHeight())
        {
            VH_ASSERT(m_InitializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Failed to begin command buffer recording");
            
            m_ColorImageView.~ImageView();
            m_ColorImage.~Image();
            m_DepthImageView.~ImageView();
            m_DepthImage.~Image();
            std::tie(m_ColorImage, m_ColorImageView, m_DepthImage, m_DepthImageView) = CreateImages(m_Device, m_Window, m_InitializationCmd, m_Renderer.GetSwapchainImageFormat());
            VH_ASSERT(m_TextureSet.AddImage(0, 0, m_ColorImageView, VulkanHelper::Image::Layout::GENERAL) == VulkanHelper::VHResult::OK, "Failed to add image to descriptor set");

            float aspect = static_cast<float>(m_Window.GetWidth()) / static_cast<float>(m_Window.GetHeight());
            projection = glm::perspective(glm::radians(38.0f), aspect, 0.1f, 100.0f);

            lastWindowSize = {m_Window.GetWidth(), m_Window.GetHeight()};

            VH_ASSERT(m_InitializationCmd.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end command buffer recording");
            VH_ASSERT(m_InitializationCmd.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit command buffer");
        }

        VulkanHelper::CommandBuffer* commandBuffer = m_Renderer.BeginFrame().Value();

        m_Renderer.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::COLOR_ATTACHMENT_OPTIMAL, *commandBuffer);

        m_RTPipeline.Bind(commandBuffer);
        m_RTPipeline.RayTrace(commandBuffer, m_Renderer.GetCurrentSwapchainImage()->GetWidth(), m_Renderer.GetCurrentSwapchainImage()->GetHeight(), 1);

        m_ColorImage.TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_SRC_OPTIMAL, *commandBuffer);
        m_Renderer.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_DST_OPTIMAL, *commandBuffer);
        VH_ASSERT(m_Renderer.GetCurrentSwapchainImage()->BlitFromImage(m_ColorImage, *commandBuffer) == VulkanHelper::VHResult::OK, "Failed to copy image");
        m_ColorImage.TransitionImageLayout(VulkanHelper::Image::Layout::GENERAL, *commandBuffer);

        VH_ASSERT(m_Renderer.EndFrame() == VulkanHelper::VHResult::OK, "Failed to end frame");
    }

    m_Device.WaitUntilIdle();
}