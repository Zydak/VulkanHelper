#pragma once

#include "Renderer/Mesh.h"
#include "Vulkan/Device.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/CommandBuffer.h"

#include <vector>
#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Mesh::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            const SharedPtr<CommandBuffer::Impl>& commandBuffer,
            Format* vertexAttributes,
            uint32_t vertexAttributeCount,
            void* vertexData,
            uint32_t vertexDataSize,
            void* indexData,
            uint32_t indexDataSize,
            VulkanHelper::Buffer::Usage AdditionalUsageFlags
        );

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Mesh& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Mesh CreatePublicInterface(const SharedPtr<Impl>& impl) { return Mesh(impl); }

        void Bind(const SharedPtr<CommandBuffer::Impl> commandBuffer) const;
        void Draw(const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t instanceCount, uint32_t firstInstance) const;

        [[nodiscard]] inline const VulkanHelper::Vector<VertexAttributeDescription>* GetAttributesDescriptions() const { return &m_VertexAttributes; };
        [[nodiscard]] VertexBindingDescription GetBindingDescription() const;

        [[nodiscard]] inline Buffer GetVertexBuffer() const { return m_VertexBuffer; }
        [[nodiscard]] inline Buffer GetIndexBuffer() const { return *m_IndexBuffer; }
        [[nodiscard]] inline uint32_t GetVertexSize() const { return m_VertexSize; }

        [[nodiscard]] static VulkanHelper::Vector<VertexAttributeDescription> CreateAttributeDescriptions(const VulkanHelper::Format* formats, uint32_t count);
        [[nodiscard]] static VertexBindingDescription CreateBindingDescription(uint32_t vertexSize);

    private:
        explicit Impl(
            const SharedPtr<Device::Impl>& device,
            Buffer&& vertexBuffer,
            UniquePtr<Buffer>&& indexBuffer,
            uint32_t vertexSize,
            VulkanHelper::Vector<Mesh::VertexAttributeDescription>&& vertexAttributes
            )
            : m_Device(device)
            , m_VertexBuffer(VulkanHelper::Move(vertexBuffer))
            , m_IndexBuffer(VulkanHelper::Move(indexBuffer))
            , m_VertexSize(vertexSize)
            , m_VertexAttributes(std::move(vertexAttributes))
        {}

        SharedPtr<Device::Impl> m_Device;
        Buffer m_VertexBuffer;
        UniquePtr<Buffer> m_IndexBuffer;

        uint32_t m_VertexSize = 0;

        VulkanHelper::Vector<Mesh::VertexAttributeDescription> m_VertexAttributes;
    };
}
