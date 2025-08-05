#pragma once

#include "Vulkan/Pipeline.h"
#include "DeviceImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class CommandBuffer;

    class Pipeline::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const GraphicsConfig& config);
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const ComputeConfig& config);
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const RayTracingConfig& config);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Pipeline* publicInterface) { return publicInterface->m_Impl.Get(); }

        void Bind(CommandBuffer* commandBuffer);
    private:
        Device::Impl* m_Device;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_Layout;
        PipelineType m_PipelineType;
        PushConstant::Impl* m_PushConstant; // Store reference to push constant for binding

        Impl(Device::Impl* device, VkPipeline pipeline, VkPipelineLayout layout, PipelineType type, PushConstant::Impl* pushConstant = nullptr)
            : m_Device(device)
            , m_Pipeline(pipeline)
            , m_Layout(layout)
            , m_PipelineType(type)
            , m_PushConstant(pushConstant)
        {}
    };
}