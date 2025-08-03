#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"

#include "Vulkan/Device.h"
#include "Vulkan/Shader.h"
#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    class Pipeline
    {
    public:
        struct GraphicsConfig
        {
            VulkanHelper::Device* Device;
            VulkanHelper::Vector<Shader*> Shaders;
			//VulkanHelper::Vector<VkVertexInputBindingDescription> BindingDesc; TODO
			//VulkanHelper::Vector<VkVertexInputAttributeDescription> AttributeDesc; TODO
			VulkanHelper::PolygonMode PolygonMode = PolygonMode::FILL;
			VulkanHelper::PrimitiveTopology Topology = VulkanHelper::PrimitiveTopology::TRIANGLE_LIST;
			VulkanHelper::CullMode CullMode = VulkanHelper::CullMode::NONE;
			bool DepthTestEnable = false;
			bool DepthClamp = false;
			bool BlendingEnable = false;
			//VulkanHelper::Vector<VkDescriptorSetLayout> DescriptorSetLayouts; // TODO
			//VkPushConstantRange* PushConstants = nullptr; TODO
			uint32_t ColorAttachmentCount = 1;
			VulkanHelper::Vector<VulkanHelper::Format> ColorFormats;
			VulkanHelper::Format DepthFormat = VulkanHelper::Format::D32_SFLOAT;
            VulkanHelper::SampleCount SampleCount = SampleCount::COUNT_1_BIT;
        };

        struct ComputeConfig
        {

        };

        struct RayTracingConfig
        {

        };

        enum class PipelineType
        {
            Graphics,
            Compute,
            RayTracing
        };

        [[nodiscard]] static Expected<Pipeline, VHResult> New(const GraphicsConfig& config);
        [[nodiscard]] static Expected<Pipeline, VHResult> New(const ComputeConfig& config);
        [[nodiscard]] static Expected<Pipeline, VHResult> New(const RayTracingConfig& config);

        Pipeline(const Pipeline& other) = delete;
        Pipeline& operator=(const Pipeline& other) = delete;

        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        ~Pipeline();

        void Bind(CommandBuffer* commandBuffer);

        class Impl;
    private:
        friend class Impl;
        UniquePtr<Impl> m_Impl;

        Pipeline(UniquePtr<Impl>&& impl);
    };
}