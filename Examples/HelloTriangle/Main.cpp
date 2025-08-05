#include "Core/Error.h"
#include "Window/Window.h"
#include "Vulkan/Instance.h"
#include "Log/Log.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Renderer/Renderer.h"
#include "Renderer/Mesh.h"

#include "Vulkan/Shader.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Buffer.h"
#include <filesystem>
#include "Vulkan/CommandPool.h"

int main()
{
    VH_LOG_INFO("Current working directory: {}", std::filesystem::current_path().c_str());
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

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", false}).Value();

    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, {&window}, &instance}).Value();

    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window, 1}).Value();

    VulkanHelper::Shader::InitializeSession("../../HelloTriangle/Shaders/");

    VulkanHelper::Shader vertexShader = VulkanHelper::Shader::New({&device, "TriangleVertex.slang", VulkanHelper::ShaderStages::VERTEX_BIT}).Value();
    VulkanHelper::Shader fragShader = VulkanHelper::Shader::New({&device, "TriangleFragment.slang", VulkanHelper::ShaderStages::FRAGMENT_BIT}).Value();

    VulkanHelper::CommandPool commandPool = VulkanHelper::CommandPool::New({&device, VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, device.GetQueueFamilyIndices().GraphicsFamily}).Value();
    VulkanHelper::CommandBuffer initializationCmd = commandPool.AllocateCommandBuffer({}).Value();
    
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
    
    VulkanHelper::Format vertexAttributes[] = {
        VulkanHelper::Format::R32G32B32_SFLOAT, // Position
        VulkanHelper::Format::R32G32B32_SFLOAT  // Color
    };
    
    VulkanHelper::Mesh::Config meshConfig{};
    meshConfig.Device = &device;
    meshConfig.VertexAttributes = vertexAttributes;
    meshConfig.VertexAttributeCount = 2;
    meshConfig.VertexData = vertices;
    meshConfig.VertexDataSize = sizeof(vertices);
    meshConfig.IndexData = indices;
    meshConfig.IndexDataSize = sizeof(indices);
    meshConfig.CommandBuffer = &initializationCmd;

    (void)initializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);
    VulkanHelper::Mesh triangleMesh = VulkanHelper::Mesh::New(meshConfig).Value();
    (void)initializationCmd.EndRecording();
    (void)initializationCmd.SubmitAndWait();

    VH_LOG_INFO("Created triangle mesh successfully!");

    VulkanHelper::Pipeline::GraphicsConfig pipelineConfig{};
    pipelineConfig.Device = &device;
    pipelineConfig.Shaders.PushBack(&vertexShader);
    pipelineConfig.Shaders.PushBack(&fragShader);
    pipelineConfig.ColorFormats.PushBack(renderer.GetSwapchainImageFormat());
    pipelineConfig.AttributeDesc = &triangleMesh.GetAttributesDescriptions();
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