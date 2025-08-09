#include "Application.h"
#include "Core/Error.h"

static std::tuple<VulkanHelper::Image, VulkanHelper::ImageView, VulkanHelper::Image, VulkanHelper::ImageView> CreateImages(VulkanHelper::Device& device, VulkanHelper::Window& window, VulkanHelper::CommandBuffer& commandBuffer)
{
    VulkanHelper::Image::Config colorImageConfig{};
    colorImageConfig.Device = &device;
    colorImageConfig.Width = window.GetWidth();
    colorImageConfig.Height = window.GetHeight();
    colorImageConfig.Format = VulkanHelper::Format::R8G8B8A8_UNORM;
    colorImageConfig.Usage = VulkanHelper::Image::Usage::COLOR_ATTACHMENT_BIT | VulkanHelper::Image::Usage::TRANSFER_SRC_BIT;
    colorImageConfig.Tiling = VulkanHelper::Image::Tiling::OPTIMAL;
    colorImageConfig.Aspect = VulkanHelper::Image::Aspect::COLOR_BIT;
    colorImageConfig.SampleCount = VulkanHelper::SampleCount::COUNT_1_BIT;
    auto outColorImage = VulkanHelper::Image::New(colorImageConfig).Value();
    outColorImage.TransitionImageLayout(
        VulkanHelper::Image::Layout::COLOR_ATTACHMENT_OPTIMAL,
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

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 800, 800, "Example Project", "", true}).Value();
    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, {&window}, &instance}).Value();
    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window}).Value();

    VulkanHelper::Shader::InitializeSession("../../../ModelViewer/Shaders/");

    VulkanHelper::Shader vertexShader = VulkanHelper::Shader::New({&device, "Vertex.slang", VulkanHelper::ShaderStages::VERTEX_BIT}).Value();
    VulkanHelper::Shader fragShader = VulkanHelper::Shader::New({&device, "Fragment.slang", VulkanHelper::ShaderStages::FRAGMENT_BIT}).Value();

    VulkanHelper::CommandPool commandPool = VulkanHelper::CommandPool::New({&device, VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, device.GetQueueFamilyIndices().GraphicsFamily}).Value();
    VulkanHelper::CommandBuffer initializationCmd = commandPool.AllocateCommandBuffer({}).Value();

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

    VH_ASSERT(initializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Failed to begin recording initialization command buffer");
    VulkanHelper::Mesh loadedMesh = VulkanHelper::Mesh::New(meshConfig).Value();

    // Mesh texture image
    VulkanHelper::Image::Config textureImageConfig{};
    textureImageConfig.Device = &device;
    textureImageConfig.Width = scene.Value().AlbedoTextures[0].Width;
    textureImageConfig.Height = scene.Value().AlbedoTextures[0].Height;
    textureImageConfig.Format = VulkanHelper::Format::R8G8B8A8_UNORM;
    textureImageConfig.Usage = VulkanHelper::Image::Usage::SAMPLED_BIT | VulkanHelper::Image::Usage::TRANSFER_DST_BIT;
    textureImageConfig.Tiling = VulkanHelper::Image::Tiling::OPTIMAL;
    textureImageConfig.Aspect = VulkanHelper::Image::Aspect::COLOR_BIT;
    VulkanHelper::Image textureImage = VulkanHelper::Image::New(textureImageConfig).Value();
    textureImage.UploadData(
        scene.Value().AlbedoTextures[0].Data.Data(),
        scene.Value().AlbedoTextures[0].Data.Size() * sizeof(uint8_t),
        0,
        &initializationCmd
    );
    textureImage.TransitionImageLayout(
        VulkanHelper::Image::Layout::SHADER_READ_ONLY_OPTIMAL,
        initializationCmd
    );

    VulkanHelper::ImageView textureImageView = VulkanHelper::ImageView::New({
        &textureImage,
        VulkanHelper::ImageView::ViewType::VIEW_2D
    }).Value();

    VulkanHelper::PushConstant pushConstant = VulkanHelper::PushConstant::New({
        VulkanHelper::ShaderStages::VERTEX_BIT,
        nullptr, // No initial data
        sizeof(glm::mat4)
    }).Value();

    // Sampler

    VulkanHelper::Sampler::Config samplerConfig{};
    samplerConfig.Device = &device;
    samplerConfig.AddressMode = VulkanHelper::Sampler::AddressMode::REPEAT;
    samplerConfig.MinFilter = VulkanHelper::Sampler::Filter::LINEAR;
    samplerConfig.MagFilter = VulkanHelper::Sampler::Filter::LINEAR;
    samplerConfig.MipmapMode = VulkanHelper::Sampler::MipmapMode::LINEAR;
    VulkanHelper::Sampler sampler = VulkanHelper::Sampler::New(samplerConfig).Value();

    // Descriptor set
    std::array<VulkanHelper::DescriptorPool::PoolSize, 2> poolSizes = {
        VulkanHelper::DescriptorPool::PoolSize{VulkanHelper::DescriptorType::SAMPLED_IMAGE, 1},
        VulkanHelper::DescriptorPool::PoolSize{VulkanHelper::DescriptorType::SAMPLER, 1}
    };
    
    VulkanHelper::DescriptorPool::Config descriptorPoolConfig{};
    descriptorPoolConfig.Device = &device;
    descriptorPoolConfig.MaxSets = 1;
    descriptorPoolConfig.PoolSizes = poolSizes.data();
    descriptorPoolConfig.PoolSizeCount = poolSizes.size();
    VulkanHelper::DescriptorPool descriptorPool = VulkanHelper::DescriptorPool::New(descriptorPoolConfig).Value();

    std::array<VulkanHelper::DescriptorSet::BindingDescription, 2> bindingDescriptions = {
        VulkanHelper::DescriptorSet::BindingDescription{0, 1, VulkanHelper::ShaderStages::FRAGMENT_BIT, VulkanHelper::DescriptorType::SAMPLED_IMAGE},
        VulkanHelper::DescriptorSet::BindingDescription{1, 1, VulkanHelper::ShaderStages::FRAGMENT_BIT, VulkanHelper::DescriptorType::SAMPLER}
    };
    VulkanHelper::DescriptorSet::Config descriptorSetConfig{};
    descriptorSetConfig.Bindings = bindingDescriptions.data();
    descriptorSetConfig.BindingCount = bindingDescriptions.size();

    VulkanHelper::DescriptorSet descriptorSet = descriptorPool.AllocateDescriptorSet(descriptorSetConfig).Value();
    VH_ASSERT(descriptorSet.AddImage(0, 0, textureImageView, VulkanHelper::Image::Layout::SHADER_READ_ONLY_OPTIMAL) == VulkanHelper::VHResult::OK, "Failed to add image to descriptor set");
    VH_ASSERT(descriptorSet.AddSampler(1, 0, sampler) == VulkanHelper::VHResult::OK, "Failed to add sampler to descriptor set");

    VulkanHelper::Pipeline::GraphicsConfig pipelineConfig{};
    pipelineConfig.Device = &device;
    pipelineConfig.Shaders.PushBack(&vertexShader);
    pipelineConfig.Shaders.PushBack(&fragShader);
    pipelineConfig.ColorFormats.PushBack(VulkanHelper::Format::R8G8B8A8_UNORM);
    pipelineConfig.AttributeDesc = loadedMesh.GetAttributesDescriptions();
    pipelineConfig.BindingDesc = loadedMesh.GetBindingDescription();
    pipelineConfig.PushConstant = &pushConstant;
    pipelineConfig.DepthTestEnable = true;
    pipelineConfig.DescriptorSets.PushBack(&descriptorSet);
    
    VulkanHelper::Pipeline pipeline = VulkanHelper::Pipeline::New(pipelineConfig).Value();

    auto [colorImage, colorImageView, depthImage, depthImageView] = CreateImages(device, window, initializationCmd);

    VH_ASSERT(initializationCmd.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end recording initialization command buffer");
    VH_ASSERT(initializationCmd.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit initialization command buffer");

    return Application(
        std::move(instance),
        std::move(window),
        std::move(device),
        std::move(renderer),
        std::move(vertexShader),
        std::move(fragShader),
        std::move(pipeline),
        std::move(loadedMesh),
        std::move(descriptorPool),
        std::move(descriptorSet),
        std::move(textureImage),
        std::move(textureImageView),
        std::move(depthImage),
        std::move(depthImageView),
        std::move(colorImage),
        std::move(colorImageView),
        std::move(sampler),
        std::move(pushConstant),
        std::move(commandPool),
        std::move(initializationCmd)
    );
}

void Application::Run()
{
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
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
            std::tie(m_ColorImage, m_ColorImageView, m_DepthImage, m_DepthImageView) = CreateImages(m_Device, m_Window, m_InitializationCmd);

            float aspect = static_cast<float>(m_Window.GetWidth()) / static_cast<float>(m_Window.GetHeight());
            projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

            lastWindowSize = {m_Window.GetWidth(), m_Window.GetHeight()};

            VH_ASSERT(m_InitializationCmd.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end command buffer recording");
            VH_ASSERT(m_InitializationCmd.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit command buffer");
        }

        VulkanHelper::CommandBuffer* commandBuffer = m_Renderer.BeginFrame().Value();

        m_Renderer.BeginRendering(*commandBuffer, {&m_ColorImageView}, &m_DepthImageView);

        m_GraphicsPipeline.Bind(commandBuffer);

        // Bind and draw the triangle mesh
        m_LoadedMesh.Bind(commandBuffer);
        m_LoadedMesh.Draw(commandBuffer);

        m_Renderer.EndRendering(*commandBuffer);

        m_ColorImage.TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_SRC_OPTIMAL, *commandBuffer);
        m_Renderer.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_DST_OPTIMAL, *commandBuffer);
        VH_ASSERT(m_Renderer.GetCurrentSwapchainImage()->BlitFromImage(m_ColorImage, *commandBuffer) == VulkanHelper::VHResult::OK, "Failed to copy image");
        m_ColorImage.TransitionImageLayout(VulkanHelper::Image::Layout::COLOR_ATTACHMENT_OPTIMAL, *commandBuffer);

        VH_ASSERT(m_Renderer.EndFrame() == VulkanHelper::VHResult::OK, "Failed to end frame");
    }

    m_Device.WaitUntilIdle();
}