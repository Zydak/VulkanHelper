#include "VulkanHelper.h"

#include <filesystem>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include <tuple>

static std::tuple<VulkanHelper::Image, VulkanHelper::ImageView, VulkanHelper::Image, VulkanHelper::ImageView> CreateImages(VulkanHelper::Device& device, VulkanHelper::Window& window)
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

int main()
{
    //
    // TEST
    //

    VulkanHelper::ThreadPool threadPool(4);

    VulkanHelper::AssetImporter importer = VulkanHelper::AssetImporter::New({&threadPool}).Value();

    auto scene = importer.ImportScene("../../../ModelViewer/Assets/VikingRoom.gltf").get();
    if (!scene.HasValue())
    {
        VH_LOG_ERROR("Failed to import mesh");
    }

    ///

    VH_LOG_INFO("Current working directory: {}", std::filesystem::current_path().string());
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({true}).Value();
    VulkanHelper::Vector<const char*> extensions;
    auto physicalDevices = instance.GetSuitablePhysicalDevices();
    if (physicalDevices.Empty())
    {
        VH_LOG_FATAL("No suitable physical devices found!");
        return -1;
    }

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

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", true}).Value();

    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, {&window}, &instance}).Value();

    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window, 2}).Value();

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

    (void)initializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);
    VulkanHelper::Mesh triangleMesh = VulkanHelper::Mesh::New(meshConfig).Value();

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

    (void)initializationCmd.EndRecording();
    (void)initializationCmd.SubmitAndWait();

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
    pipelineConfig.AttributeDesc = triangleMesh.GetAttributesDescriptions();
    pipelineConfig.BindingDesc = triangleMesh.GetBindingDescription();
    pipelineConfig.PushConstant = &pushConstant;
    pipelineConfig.DepthTestEnable = true;
    pipelineConfig.DescriptorSets.PushBack(&descriptorSet);
    
    VulkanHelper::Pipeline pipeline = VulkanHelper::Pipeline::New(pipelineConfig).Value();

    auto [colorImage, colorImageView, depthImage, depthImageView] = CreateImages(device, window);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for model transformation
    model = glm::translate(model, glm::vec3(0.0f, 0.25f, 0.0f));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Blender has some really weird coordinate system
    auto timer = std::chrono::high_resolution_clock::now();
    while (!window.WantsToClose())
    {
        {
            float time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - timer).count();

            glm::vec3 position = glm::vec3(glm::sin(time / 2.0f) * 3.0f, -1.5f, glm::cos(time / 2.0f) * 3.0f);
            (void)position;
            glm::mat4 view = glm::lookAt(
                position,
                glm::vec3(0.0f, 0.0f, 0.0f), // Look at point
                glm::vec3(0.0f, 1.0f, 0.0f)  // Up vector
            );
            (void) view;
            glm::mat4 mvp = projection * view * model;

            VH_ASSERT(pushConstant.SetData(&mvp, sizeof(mvp)) == VulkanHelper::VHResult::OK, "Failed to set push constant data");
        }

        VulkanHelper::Window::PollEvents();
        static glm::vec2 lastWindowSize = {window.GetWidth(), window.GetHeight()};
        if (lastWindowSize.x != window.GetWidth() || lastWindowSize.y != window.GetHeight())
        {
            colorImageView.~ImageView();
            colorImage.~Image();
            depthImageView.~ImageView();
            depthImage.~Image();
            std::tie(colorImage, colorImageView, depthImage, depthImageView) = CreateImages(device, window);

            float aspect = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
            projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

            lastWindowSize = {window.GetWidth(), window.GetHeight()};
        }


        VulkanHelper::CommandBuffer* commandBuffer = renderer.BeginFrame().Value();

        renderer.BeginRendering(*commandBuffer, {&colorImageView}, &depthImageView);

        pipeline.Bind(commandBuffer);

        // Bind and draw the triangle mesh
        triangleMesh.Bind(commandBuffer);
        triangleMesh.Draw(commandBuffer);

        renderer.EndRendering(*commandBuffer);

        colorImage.TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_SRC_OPTIMAL, *commandBuffer);
        renderer.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_DST_OPTIMAL, *commandBuffer);
        VH_ASSERT(renderer.GetCurrentSwapchainImage()->BlitFromImage(colorImage, *commandBuffer) == VulkanHelper::VHResult::OK, "Failed to copy image");
        colorImage.TransitionImageLayout(VulkanHelper::Image::Layout::COLOR_ATTACHMENT_OPTIMAL, *commandBuffer);

        VH_ASSERT(renderer.EndFrame() == VulkanHelper::VHResult::OK, "Failed to end frame");
    }

    device.WaitUntilIdle();

    return 0;
}