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
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(const Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Mesh* publicInterface) 
        { 
            return publicInterface->m_Impl.Get();
        }

        void Bind(CommandBuffer* commandBuffer) const;
        void Draw(CommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t firstInstance) const;

        [[nodiscard]] const VulkanHelper::Vector<VertexAttributeDescription>& GetAttributesDescriptions() const;
        [[nodiscard]] VertexBindingDescription GetBindingDescription() const;

    private:
        explicit Impl(Device::Impl* device,
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

        Device::Impl* m_Device;
        Buffer m_VertexBuffer;
        UniquePtr<Buffer> m_IndexBuffer;

        uint32_t m_VertexSize = 0;

        VulkanHelper::Vector<Mesh::VertexAttributeDescription> m_VertexAttributes;
    };
}
