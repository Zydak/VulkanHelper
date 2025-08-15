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
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            const Vector<SharedPtr<Shader::Impl>>& shaders,
            const Vector<Mesh::VertexAttributeDescription>* attributeDesc,
            Mesh::VertexBindingDescription bindingDesc,
            PolygonMode polygonMode,
            PrimitiveTopology topology,
            CullMode cullMode,
            bool depthTestEnable,
            bool depthClamp,
            bool blendingEnable,
            Vector<SharedPtr<DescriptorSet::Impl>>&& descriptorSets,
            const SharedPtr<PushConstant::Impl>& pushConstant,
            uint32_t colorAttachmentCount,
            Vector<Format>&& colorFormats,
            Format depthFormat,
            SampleCount sampleCount
        );

        // Compute
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            const SharedPtr<Shader::Impl>& computeShader,
            Vector<SharedPtr<DescriptorSet::Impl>>&& descriptorSets,
            const SharedPtr<PushConstant::Impl>& pushConstant
        );

        // Ray Tracing
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            const Vector<SharedPtr<Shader::Impl>>& RayGenShaders,
            const Vector<SharedPtr<Shader::Impl>>& HitShaders,
            const Vector<SharedPtr<Shader::Impl>>& MissShaders,
            Vector<SharedPtr<DescriptorSet::Impl>>&& descriptorSets,
            const SharedPtr<PushConstant::Impl>& pushConstant,
            const SharedPtr<CommandBuffer::Impl>& cmd
        );

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Pipeline& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Pipeline CreatePublicInterface(const SharedPtr<Impl>& impl) { return Pipeline(impl); }

        void Bind(const SharedPtr<CommandBuffer::Impl>& commandBuffer);

        void Dispatch(const SharedPtr<CommandBuffer::Impl>& commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        void RayTrace(const SharedPtr<CommandBuffer::Impl>& commandBuffer, uint32_t width, uint32_t height, uint32_t depth = 1);

        [[nodiscard]] inline VkPipeline GetPipeline() const { return m_Pipeline; }
        [[nodiscard]] inline VkPipelineLayout GetLayout() const { return m_Layout; }
        [[nodiscard]] inline PipelineType GetPipelineType() const { return m_PipelineType; }

        [[nodiscard]] inline SharedPtr<Device::Impl> GetDevice() const { return m_Device; }

        [[nodiscard]] inline const Vector<SharedPtr<DescriptorSet::Impl>>& GetDescriptorSets() const { return m_DescriptorSets; }
        [[nodiscard]] inline SharedPtr<PushConstant::Impl> GetPushConstant() const { return m_PushConstant; }

    private:
        [[nodiscard]] static Expected<VkPipelineLayout, VHResult> CreatePipelineLayout(
            const SharedPtr<Device::Impl>& device,
            const Vector<SharedPtr<DescriptorSet::Impl>>& descriptorSets,
            const SharedPtr<PushConstant::Impl>& pushConstant
        );

        SharedPtr<Device::Impl> m_Device;
        VkPipeline m_Pipeline;
        VkPipelineLayout m_Layout;
        PipelineType m_PipelineType;
        Vector<SharedPtr<DescriptorSet::Impl>> m_DescriptorSets; // Store references to descriptor sets for binding
        SharedPtr<PushConstant::Impl> m_PushConstant; // Store reference to push constant for binding
        UniquePtr<SBT> m_SBT; // Optional SBT for ray tracing pipelines
        Vector<SharedPtr<Shader::Impl>> m_Shaders; // Store shaders for graphics and compute pipelines

        Impl(
            const SharedPtr<Device::Impl>& device,
            VkPipeline pipeline,
            VkPipelineLayout layout,
            PipelineType type,
            Vector<SharedPtr<DescriptorSet::Impl>>&& descriptorSets,
            const SharedPtr<PushConstant::Impl>& pushConstant,
            UniquePtr<SBT>&& sbt,
            const Vector<SharedPtr<Shader::Impl>>& shaders
        )
            : m_Device(device)
            , m_Pipeline(pipeline)
            , m_Layout(layout)
            , m_PipelineType(type)
            , m_DescriptorSets(VulkanHelper::Move(descriptorSets))
            , m_PushConstant(pushConstant)
            , m_SBT(Move(sbt))
            , m_Shaders(shaders.Clone())
        {}
    };
}