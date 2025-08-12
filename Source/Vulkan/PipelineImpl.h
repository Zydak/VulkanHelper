#pragma once

#include "Vulkan/Pipeline.h"
#include "DeviceImpl.h"
#include "DescriptorSetImpl.h"
#include "SBT.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class CommandBuffer;

    class Pipeline::Impl
    {
    public:
        // Graphics
        [[nodiscard]] static Expected<Impl, VHResult> New(
            Device::Impl* device,
            Vector<Shader::Impl*>&& shaders,
            const Vector<Mesh::VertexAttributeDescription>* attributeDesc,
            Mesh::VertexBindingDescription bindingDesc,
            PolygonMode polygonMode,
            PrimitiveTopology topology,
            CullMode cullMode,
            bool depthTestEnable,
            bool depthClamp,
            bool blendingEnable,
            Vector<DescriptorSet::Impl*>&& descriptorSets,
            PushConstant::Impl* pushConstant,
            uint32_t colorAttachmentCount,
            Vector<Format>&& colorFormats,
            Format depthFormat,
            SampleCount sampleCount
        );

        // Compute
        [[nodiscard]] static Expected<Impl, VHResult> New(
            Device::Impl* device,
            Shader::Impl* computeShader,
            Vector<DescriptorSet::Impl*>&& descriptorSets,
            PushConstant::Impl* pushConstant
        );

        // Ray Tracing
        [[nodiscard]] static Expected<Impl, VHResult> New(
            Device::Impl* device,
            Vector<Shader::Impl*>&& RayGenShaders,
            Vector<Shader::Impl*>&& HitShaders,
            Vector<Shader::Impl*>&& MissShaders,
            Vector<DescriptorSet::Impl*>&& descriptorSets,
            PushConstant::Impl* pushConstant,
            CommandBuffer::Impl* cmd
        );

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Pipeline* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Pipeline CreatePublicInterface(Impl&& impl) { return Pipeline(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<Pipeline> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new Pipeline(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        void Bind(CommandBuffer::Impl* commandBuffer);

        void Dispatch(CommandBuffer::Impl* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void RayTrace(CommandBuffer::Impl* commandBuffer, uint32_t width, uint32_t height, uint32_t depth = 1);

        [[nodiscard]] inline VkPipeline GetPipeline() const { return m_Pipeline; }
        [[nodiscard]] inline VkPipelineLayout GetLayout() const { return m_Layout; }
        [[nodiscard]] inline PipelineType GetPipelineType() const { return m_PipelineType; }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }

        [[nodiscard]] inline const Vector<DescriptorSet::Impl*>& GetDescriptorSets() const { return m_DescriptorSets; }
        [[nodiscard]] inline PushConstant::Impl* GetPushConstant() const { return m_PushConstant; }

    private:
        [[nodiscard]] static Expected<VkPipelineLayout, VHResult> CreatePipelineLayout(Device::Impl* device, const Vector<DescriptorSet::Impl*>& descriptorSets, PushConstant::Impl* pushConstant);

        Device::Impl* m_Device;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_Layout;
        PipelineType m_PipelineType;
        Vector<DescriptorSet::Impl*> m_DescriptorSets; // Store references to descriptor sets for binding
        PushConstant::Impl* m_PushConstant; // Store reference to push constant for binding
        UniquePtr<SBT> m_SBT; // Optional SBT for ray tracing pipelines

        Impl(
            Device::Impl* device,
            VkPipeline pipeline,
            VkPipelineLayout layout,
            PipelineType type,
            Vector<DescriptorSet::Impl*>&& descriptorSets,
            PushConstant::Impl* pushConstant,
            UniquePtr<SBT>&& sbt
        )
            : m_Device(device)
            , m_Pipeline(pipeline)
            , m_Layout(layout)
            , m_PipelineType(type)
            , m_DescriptorSets(VulkanHelper::Move(descriptorSets))
            , m_PushConstant(pushConstant)
            , m_SBT(Move(sbt))
        {}
    };
}