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
    for (size_t i = 0; i < physicalDevices.Size(); i++)
    {
        VH_LOG_INFO("Found Physical Device: {} (Vendor: {}, Discrete: {})", physicalDevices[i].GetName(), int(physicalDevices[i].GetVendor()), physicalDevices[i].IsDiscrete());
    }

    // Pick discrete GPU if available
    VulkanHelper::PhysicalDevice* selectedDevice = nullptr;
    for (size_t i = 0; i < physicalDevices.Size(); i++)
    {   
        if (physicalDevices[i].IsDiscrete())
        {
            VH_LOG_INFO("Selected Discrete GPU: {}", physicalDevices[i].GetName());
            selectedDevice = &physicalDevices[i];
            break;
        }
    }
    if (selectedDevice == nullptr)
    {
        VH_LOG_WARN("No discrete GPU found, using first available device: {}", physicalDevices[0].GetName());
        selectedDevice = &physicalDevices[0];
    }

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", false}).Value();

    VulkanHelper::Vector<VulkanHelper::Window*> windows;
    windows.PushBack(&window);

    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, std::move(windows), &instance}).Value();

    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window, 1}).Value();

    VulkanHelper::Shader::InitializeSession("../../ExampleProject/Shaders/");

    VulkanHelper::Shader vertexShader = VulkanHelper::Shader::New({&device, "TriangleVertex.slang", VulkanHelper::ShaderStages::VERTEX_BIT}).Value();
    VulkanHelper::Shader fragShader = VulkanHelper::Shader::New({&device, "TriangleFragment.slang", VulkanHelper::ShaderStages::FRAGMENT_BIT}).Value();

    VulkanHelper::CommandPool commandPool = VulkanHelper::CommandPool::New({&device, VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, device.GetQueueFamilyIndices().GraphicsFamily}).Value();
    VulkanHelper::CommandBuffer testCmdBuffer = commandPool.AllocateCommandBuffer({}).Value();
    (void)testCmdBuffer.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);

    VulkanHelper::Buffer::Usage usage = VulkanHelper::Buffer::Usage::TRANSFER_SRC | VulkanHelper::Buffer::Usage::TRANSFER_DST;
    VulkanHelper::Buffer buffer = VulkanHelper::Buffer::New({&device, nullptr, 5, usage, false}).Value();
    int data = 59; 
    (void)buffer.UploadData(&data, 4, 1, &testCmdBuffer);

    int data1;
    (void)buffer.DownloadData(&data1, 4, 1, &testCmdBuffer);
    (void)testCmdBuffer.EndRecording();
    (void)testCmdBuffer.SubmitAndWait();

    VH_LOG_FATAL("DATA: {}", data1);

    data = 420;

    VulkanHelper::Image::Config imageInfo{};
    imageInfo.Device = &device;
    imageInfo.Width = 1;
    imageInfo.Height = 1;
    imageInfo.Format = VulkanHelper::Format::R8G8B8A8_UNORM;
    imageInfo.Usage = VulkanHelper::Image::Usage::TRANSFER_SRC_BIT | VulkanHelper::Image::Usage::TRANSFER_DST_BIT;
    imageInfo.UsePersistentStagingBuffer = false;
    //imageInfo.Tiling = VulkanHelper::Image::Tiling::LINEAR;
    //imageInfo.AllowMapping = true;
    VulkanHelper::Image testImage = VulkanHelper::Image::New(imageInfo).Value();
    (void)testCmdBuffer.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);

    (void)testImage.UploadData(&data, 4, 0, &testCmdBuffer);

    (void)testImage.TransitionImageLayout(VulkanHelper::Image::Layout::TRANSFER_SRC_OPTIMAL, testCmdBuffer);
    (void)testImage.DownloadData(&data1, 4, 0, &testCmdBuffer);
    (void)testCmdBuffer.EndRecording();
    (void)testCmdBuffer.SubmitAndWait();
    VH_LOG_FATAL("DATA: {}", data1);
    
    // Create a simple triangle mesh for demonstration
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
    meshConfig.CommandBuffer = &testCmdBuffer;
    
    (void)testCmdBuffer.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);
    VulkanHelper::Mesh triangleMesh = VulkanHelper::Mesh::New(meshConfig).Value();
    (void)testCmdBuffer.EndRecording();
    (void)testCmdBuffer.SubmitAndWait();

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

        VulkanHelper::Vector<VulkanHelper::ImageView*> views;
        views.PushBack(renderer.GetCurrentSwapchainImageView());
        renderer.BeginRendering(*commandBuffer, views, nullptr);

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