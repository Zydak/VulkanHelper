#pragma once

#include "VulkanHelper.h"

class Application
{
public:
    static Application New();

    ~Application() = default;

    void Run();

private:

    Application(
        VulkanHelper::Instance&& instance,
        VulkanHelper::Device&& device,
        VulkanHelper::Window&& window,
        VulkanHelper::Renderer&& renderer,
        VulkanHelper::Shader&& computeShader,
        VulkanHelper::Pipeline&& pipeline,
        VulkanHelper::Mesh&& points,
        VulkanHelper::DescriptorPool&& descriptorPool,
        VulkanHelper::DescriptorSet&& computeSet,
        VulkanHelper::Shader&& vertexShader,
        VulkanHelper::Shader&& fragmentShader,
        VulkanHelper::Pipeline&& graphicsPipeline
    )
    : m_Instance(std::move(instance)),
      m_Device(std::move(device)),
      m_Window(std::move(window)),
      m_Renderer(std::move(renderer)),
      m_ComputeShader(std::move(computeShader)),
      m_Pipeline(std::move(pipeline)),
      m_Points(std::move(points)),
      m_DescriptorPool(std::move(descriptorPool)),
      m_ComputeSet(std::move(computeSet)),
      m_VertexShader(std::move(vertexShader)),
      m_FragmentShader(std::move(fragmentShader)),
      m_GraphicsPipeline(std::move(graphicsPipeline))
    {}

    static constexpr uint32_t POINTS_COUNT = 10'000;

    VulkanHelper::Instance m_Instance;
    VulkanHelper::Device m_Device;
    VulkanHelper::Window m_Window;
    VulkanHelper::Renderer m_Renderer;

    VulkanHelper::Shader m_ComputeShader;
    VulkanHelper::Pipeline m_Pipeline;

    VulkanHelper::Mesh m_Points;

    VulkanHelper::DescriptorPool m_DescriptorPool;
    VulkanHelper::DescriptorSet m_ComputeSet;

    VulkanHelper::Shader m_VertexShader;
    VulkanHelper::Shader m_FragmentShader;
    VulkanHelper::Pipeline m_GraphicsPipeline;
};