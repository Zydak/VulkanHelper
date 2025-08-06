#pragma once

#include "Vulkan/Pipeline.h"
#include "DeviceImpl.h"
#include "DescriptorSetImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class CommandBuffer;

    class Pipeline::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(Device::Impl* device, Vector<Shader::Impl*> shaders, const Vector<Mesh::VertexAttributeDescription>* attributeDesc, Mesh::VertexBindingDescription bindingDesc, PolygonMode polygonMode, PrimitiveTopology topology, CullMode cullMode, bool depthTestEnable, bool depthClamp, bool blendingEnable, Vector<DescriptorSet::Impl*> descriptorSets, PushConstant::Impl* pushConstant, uint32_t colorAttachmentCount, Vector<Format> colorFormats, Format depthFormat, SampleCount sampleCount);
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const ComputeConfig& config);
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const RayTracingConfig& config);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Pipeline* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Pipeline CreatePublicInterface(UniquePtr<Impl>&& impl) { return Pipeline(VulkanHelper::Move(impl)); }

        void Bind(CommandBuffer* commandBuffer);

        void Dispatch(CommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        [[nodiscard]] inline VkPipeline GetPipeline() const { return m_Pipeline; }
        [[nodiscard]] inline VkPipelineLayout GetLayout() const { return m_Layout; }
        [[nodiscard]] inline PipelineType GetPipelineType() const { return m_PipelineType; }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }

        [[nodiscard]] inline const Vector<DescriptorSet::Impl*>& GetDescriptorSets() const { return m_DescriptorSets; }
        [[nodiscard]] inline PushConstant::Impl* GetPushConstant() const { return m_PushConstant; }

    private:
        Device::Impl* m_Device;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_Layout;
        PipelineType m_PipelineType;
        Vector<DescriptorSet::Impl*> m_DescriptorSets; // Store references to descriptor sets for binding
        PushConstant::Impl* m_PushConstant; // Store reference to push constant for binding

        Impl(Device::Impl* device, VkPipeline pipeline, VkPipelineLayout layout, PipelineType type, Vector<DescriptorSet::Impl*>&& descriptorSets, PushConstant::Impl* pushConstant = nullptr)
            : m_Device(device)
            , m_Pipeline(pipeline)
            , m_Layout(layout)
            , m_PipelineType(type)
            , m_DescriptorSets(VulkanHelper::Move(descriptorSets))
            , m_PushConstant(pushConstant)
        {}
    };
}