#include "Application.h"

#include <filesystem>
#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

struct Particle
{
    glm::vec2 Position;
    glm::vec2 Velocity;
};

Application Application::New()
{
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({true}).Value();
    VulkanHelper::Vector<const char*> extensions;
    auto physicalDevices = instance.GetSuitablePhysicalDevices();
    if (physicalDevices.Empty())
    {
        VH_LOG_FATAL("No suitable physical devices found!");
        throw -1;
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

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "NBodySim Example", "", true}).Value();

    VulkanHelper::Vector<VulkanHelper::Window*> windows;
    windows.PushBack(&window);

    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, std::move(windows), &instance}).Value();

    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window}).Value();

    VulkanHelper::CommandPool::Config cmdPoolConfig;
    cmdPoolConfig.Device = &device;
    cmdPoolConfig.Flags = VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT;
    cmdPoolConfig.QueueFamilyIndex = device.GetQueueFamilyIndices().GraphicsFamily;
    VulkanHelper::CommandPool cmdPool = VulkanHelper::CommandPool::New(cmdPoolConfig).Value();
    VulkanHelper::CommandBuffer initializationCmd = cmdPool.AllocateCommandBuffer({}).Value();
    VH_ASSERT(initializationCmd.BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Failed to begin initialization command buffer!");

    //
    //  Initial Points Positions
    //
    std::vector<Particle> particles(POINTS_COUNT);
    for (uint32_t i = 0; i < POINTS_COUNT; ++i)
    {
        particles[i].Position = glm::vec2(
            static_cast<float>(rand() % 100) + rand() / static_cast<float>(RAND_MAX),
            static_cast<float>(rand() % 100) + rand() / static_cast<float>(RAND_MAX)
        );
        particles[i].Velocity = glm::vec2(0.0, 0.0);
    }

    // Mesh
    VulkanHelper::Format vertexAttributes[] = {
        VulkanHelper::Format::R32G32_SFLOAT, // Position
        VulkanHelper::Format::R32G32_SFLOAT  // Velocity
    };
    
    VulkanHelper::Mesh::Config pointsMeshConfig;
    pointsMeshConfig.Device = &device;
    pointsMeshConfig.VertexDataSize = POINTS_COUNT * sizeof(Particle);
    pointsMeshConfig.VertexData = particles.data();
    pointsMeshConfig.VertexAttributeCount = 2;
    pointsMeshConfig.VertexAttributes = vertexAttributes;
    pointsMeshConfig.CommandBuffer = &initializationCmd;
    pointsMeshConfig.AdditionalUsageFlags = VulkanHelper::Buffer::Usage::STORAGE_BUFFER;
    VulkanHelper::Mesh pointsMesh = VulkanHelper::Mesh::New(pointsMeshConfig).Value();
    
    //
    //  Descriptor Sets
    //

    // Descriptor Pool
    VulkanHelper::DescriptorPool::PoolSize poolSizes[] = {
        {VulkanHelper::DescriptorType::STORAGE_BUFFER, 1}
    };
    VulkanHelper::DescriptorPool::Config descriptorPoolConfig;
    descriptorPoolConfig.Device = &device;
    descriptorPoolConfig.MaxSets = 1;
    descriptorPoolConfig.PoolSizes = poolSizes;
    descriptorPoolConfig.PoolSizeCount = 1;

    VulkanHelper::DescriptorPool descriptorPool = VulkanHelper::DescriptorPool::New(descriptorPoolConfig).Value();

    // Descriptor Set
    VulkanHelper::DescriptorSet::BindingDescription bindings[] = {
        {0, 1, VulkanHelper::ShaderStages::COMPUTE_BIT, VulkanHelper::DescriptorType::STORAGE_BUFFER}
    };
    VulkanHelper::DescriptorSet::Config computeSetConfig;
    computeSetConfig.Bindings = bindings;
    computeSetConfig.BindingCount = 1;
    VulkanHelper::DescriptorSet computeSet = descriptorPool.AllocateDescriptorSet(computeSetConfig).Value();
    VH_ASSERT(computeSet.AddBuffer(0, 0, *pointsMesh.GetVertexBuffer()) == VulkanHelper::VHResult::OK, "Failed to add buffer to compute descriptor set!");

    //
    //  Compute Pipeline
    //

    // Shader
    VulkanHelper::Shader::InitializeSession("../../../NBodySim/Shaders/");
    VulkanHelper::Shader computeShader = VulkanHelper::Shader::New({&device, "Compute.slang", VulkanHelper::ShaderStages::COMPUTE_BIT}).Value();

    // Pipeline
    VulkanHelper::Pipeline::ComputeConfig pipelineConfig;
    pipelineConfig.Device = &device;
    pipelineConfig.ComputeShader = &computeShader;
    pipelineConfig.DescriptorSets.PushBack(&computeSet);

    VulkanHelper::Pipeline pipeline = VulkanHelper::Pipeline::New(pipelineConfig).Value();

    //
    //  Graphics Pipeline
    //

    // Shaders
    VulkanHelper::Shader vertexShader = VulkanHelper::Shader::New({&device, "Vertex.slang", VulkanHelper::ShaderStages::VERTEX_BIT}).Value();
    VulkanHelper::Shader fragmentShader = VulkanHelper::Shader::New({&device, "Fragment.slang", VulkanHelper::ShaderStages::FRAGMENT_BIT}).Value();
    VulkanHelper::Pipeline::GraphicsConfig graphicsPipelineConfig;
    graphicsPipelineConfig.Device = &device;
    graphicsPipelineConfig.Shaders.PushBack(&vertexShader);
    graphicsPipelineConfig.Shaders.PushBack(&fragmentShader);

    graphicsPipelineConfig.AttributeDesc = pointsMesh.GetAttributesDescriptions();
    graphicsPipelineConfig.BindingDesc = pointsMesh.GetBindingDescription();
    graphicsPipelineConfig.PolygonMode = VulkanHelper::PolygonMode::FILL;
    graphicsPipelineConfig.Topology = VulkanHelper::PrimitiveTopology::POINT_LIST;
    graphicsPipelineConfig.ColorFormats.PushBack(renderer.GetSwapchainImageFormat());

    VulkanHelper::Pipeline graphicsPipeline = VulkanHelper::Pipeline::New(graphicsPipelineConfig).Value();

    // Finish command buffer recording
    VH_ASSERT(initializationCmd.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end initialization command buffer!");
    VH_ASSERT(initializationCmd.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit initialization command buffer!");

    return Application(
        std::move(instance),
        std::move(device),
        std::move(window),
        std::move(renderer),
        std::move(computeShader),
        std::move(pipeline),
        std::move(pointsMesh),
        std::move(descriptorPool),
        std::move(computeSet),
        std::move(vertexShader),
        std::move(fragmentShader),
        std::move(graphicsPipeline)
    );
}

void Application::Run()
{
    while (!m_Window.WantsToClose())
    {
        m_Window.PollEvents();
        
        VulkanHelper::CommandBuffer* cmd = m_Renderer.BeginFrame().Value();

        m_Pipeline.Bind(cmd);
        m_Pipeline.Dispatch(cmd, POINTS_COUNT / 32 + 1, 1, 1);

        m_Points.GetVertexBuffer()->Barrier(
            *cmd,
            VulkanHelper::AccessFlags::SHADER_WRITE_BIT,
            VulkanHelper::AccessFlags::VERTEX_ATTRIBUTE_READ_BIT,
            VulkanHelper::PipelineStages::COMPUTE_SHADER_BIT,
            VulkanHelper::PipelineStages::VERTEX_INPUT_BIT
        );

        m_Renderer.BeginRendering(*cmd, {m_Renderer.GetCurrentSwapchainImageView()}, nullptr, {0.1f, 0.1f, 0.1f, 1.0f});
        m_GraphicsPipeline.Bind(cmd);
        m_Points.Bind(cmd);
        m_Points.Draw(cmd);
        m_Renderer.EndRendering(*cmd);

        VH_ASSERT(m_Renderer.EndFrame() == VulkanHelper::VHResult::OK, "Failed to end frame!");
    }

    m_Device.WaitUntilIdle();
}