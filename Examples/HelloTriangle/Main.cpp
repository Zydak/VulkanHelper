#include "VulkanHelper.h"

#include <array>

int main()
{
    // Initialize Vulkan instance and select a suitable physical device
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

    // Create a window for rendering
    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", true}).Value();

    // Create a Vulkan logical device
    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, {&window}, &instance}).Value();

    // Create a renderer for the window
    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window}).Value();

    // Compile shaders
    VulkanHelper::Shader::InitializeSession("../../../HelloTriangle/Shaders/");
    VulkanHelper::Shader vertexShader = VulkanHelper::Shader::New({&device, "TriangleVertex.slang", VulkanHelper::ShaderStages::VERTEX_BIT}).Value();
    VulkanHelper::Shader fragShader = VulkanHelper::Shader::New({&device, "TriangleFragment.slang", VulkanHelper::ShaderStages::FRAGMENT_BIT}).Value();

    // Command buffer is needed for some initialization tasks, it has to be submitted before rendering
    VulkanHelper::CommandPool commandPool = VulkanHelper::CommandPool::New({&device, VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, device.GetQueueFamilyIndices().GraphicsFamily}).Value();
    VulkanHelper::CommandBuffer initializationCmd = commandPool.AllocateCommandBuffer({}).Value();
    VH_ASSERT(initializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Failed to begin recording initialization command buffer");
    
    struct Vertex {
        float position[3];
        float color[3];
    };
    
    Vertex vertices[] = {
        {{ 0.0f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Top - Blue
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}}, // Bottom left - Green  
        {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}  // Bottom right - Red
    };
    
    uint32_t indices[] = {0, 1, 2};

    std::array<VulkanHelper::Format, 2> vertexAttributes = {
        VulkanHelper::Format::R32G32B32_SFLOAT, // Position
        VulkanHelper::Format::R32G32B32_SFLOAT  // Color
    };
    
    VulkanHelper::Mesh::Config meshConfig{};
    meshConfig.Device = &device;
    meshConfig.VertexAttributes = vertexAttributes.data();
    meshConfig.VertexAttributeCount = vertexAttributes.size();
    meshConfig.VertexData = vertices;
    meshConfig.VertexDataSize = sizeof(Vertex) * 3;
    meshConfig.IndexData = indices;
    meshConfig.IndexDataSize = sizeof(uint32_t) * 3;
    meshConfig.CommandBuffer = &initializationCmd;

    VulkanHelper::Mesh triangleMesh = VulkanHelper::Mesh::New(meshConfig).Value();

    // Submit the command buffer to initialize the mesh
    VH_ASSERT(initializationCmd.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end recording initialization command buffer");
    VH_ASSERT(initializationCmd.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit initialization command buffer");

    // Create a graphics pipeline for rendering the triangle
    VulkanHelper::Pipeline::GraphicsConfig pipelineConfig{};
    pipelineConfig.Device = &device;
    pipelineConfig.Shaders.PushBack(&vertexShader);
    pipelineConfig.Shaders.PushBack(&fragShader);
    pipelineConfig.ColorFormats.PushBack(renderer.GetSwapchainImageFormat());
    pipelineConfig.AttributeDesc = triangleMesh.GetAttributesDescriptions();
    pipelineConfig.BindingDesc = triangleMesh.GetBindingDescription();
    
    VulkanHelper::Pipeline pipeline = VulkanHelper::Pipeline::New(pipelineConfig).Value();
    
    while (!window.WantsToClose())
    {
        VulkanHelper::Window::PollEvents();
        VulkanHelper::CommandBuffer* commandBuffer = renderer.BeginFrame().Value();

        renderer.BeginRendering(*commandBuffer, {renderer.GetCurrentSwapchainImageView()}, nullptr);

        pipeline.Bind(commandBuffer);

        // Bind and draw the triangle mesh
        triangleMesh.Bind(commandBuffer);
        triangleMesh.Draw(commandBuffer);

        renderer.EndRendering(*commandBuffer);

        VH_ASSERT(renderer.EndFrame() == VulkanHelper::VHResult::OK, "Failed to end frame");
    }

    device.WaitUntilIdle();

    return 0;
}