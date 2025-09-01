#include "MeshImpl.h"
#include "Vulkan/DeviceImpl.h"
#include "Vulkan/CommandBufferImpl.h"
#include "Vulkan/BufferImpl.h"
#include "Log/Log.h"
#include "Core/Move.h"

#include <vulkan/vulkan.h>

#include "Utility/Utility.h"

namespace VulkanHelper
{
    Expected<SharedPtr<Mesh::Impl>, VHResult> Mesh::Impl::New(
        const SharedPtr<Device::Impl>& device,
        const SharedPtr<CommandBuffer::Impl>& commandBuffer,
        Format* vertexAttributes,
        uint32_t vertexAttributeCount,
        void* vertexData,
        uint32_t vertexDataSize,
        void* indexData,
        uint32_t indexDataSize,
        VulkanHelper::Buffer::Usage AdditionalUsageFlags
    )
    {
        if (!vertexAttributes || vertexAttributeCount == 0)
        {
            VH_LOG_ERROR("VertexAttributes cannot be null and VertexAttributeCount must be greater than 0");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (!commandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Create vertex buffer
        auto vertexBufferResult = Buffer::Impl::New(
            device,
            vertexDataSize,
            Buffer::Usage::VERTEX_BUFFER_BIT | Buffer::Usage::TRANSFER_DST_BIT | AdditionalUsageFlags,
            false, // CpuMapable
            false, // UsePersistentStagingBuffer
            1, // Minimum alignment
            "Mesh Vertex Buffer"
        );
        if (!vertexBufferResult.HasValue())
        {
            VH_LOG_ERROR("Failed to create vertex buffer");
            return Unexpected(vertexBufferResult.Error());
        }

        Buffer vertexBuffer = Buffer::Impl::CreatePublicInterface(Move(vertexBufferResult.Value()));

        // Upload vertex data
        if (vertexData != nullptr)
        {
            VHResult uploadResult = Buffer::Impl::GetImplementation(vertexBuffer)->UploadData(vertexData, vertexDataSize, 0, commandBuffer);
            if (uploadResult != VHResult::OK)
            {
                VH_LOG_ERROR("Failed to upload vertex data to buffer");
                return Unexpected(uploadResult);
            }
        }

        // Calculate vertex count based on vertex attribute sizes
        VulkanHelper::Vector<Mesh::VertexAttributeDescription> vertexAttributesVec;
        uint32_t vertexSize = 0;
        for (uint32_t i = 0; i < vertexAttributeCount; ++i)
        {
            Format format = vertexAttributes[i];
            uint32_t formatSize = VulkanHelper::GetFormatSize(format);

            Mesh::VertexAttributeDescription attr{};
            attr.Location = i;
            attr.Binding = 0; // Assuming single binding for simplicity
            attr.Format = format;
            attr.Offset = vertexSize;
            vertexAttributesVec.PushBack(Move(attr));

            vertexSize += formatSize;
        }

        uint32_t vertexCount = vertexDataSize / vertexSize;
        if (vertexDataSize % vertexSize != 0)
        {
            VH_LOG_ERROR("VertexDataSize ({}) is not evenly divisible by vertex size ({}), Make sure you passed the correct data!", vertexDataSize, vertexSize);
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Create index buffer if index data is provided
        UniquePtr<Buffer> indexBuffer(nullptr);
        uint32_t indexCount = 0;
        
        if (indexData && indexDataSize > 0)
        {
            auto indexBufferResult = Buffer::Impl::New(
                device,
                indexDataSize,
                Buffer::Usage::INDEX_BUFFER_BIT | Buffer::Usage::TRANSFER_DST_BIT | AdditionalUsageFlags,
                false, // CpuMapable
                false, // UsePersistentStagingBuffer
                1, // Minimum alignment
                "Mesh Index Buffer"
            );
            if (!indexBufferResult.HasValue())
            {
                VH_LOG_ERROR("Failed to create index buffer");
                return Unexpected(indexBufferResult.Error());
            }

            indexBuffer = new Buffer(Buffer::Impl::CreatePublicInterface(indexBufferResult.Value()));

            // Upload index data
            VHResult uploadResult = Buffer::Impl::GetImplementation(*indexBuffer)->UploadData(indexData, indexDataSize, 0, commandBuffer);
            if (uploadResult != VHResult::OK)
            {
                VH_LOG_ERROR("Failed to upload index data to buffer");
                return Unexpected(uploadResult);
            }

            // Assume 32-bit indices (could be extended to support 16-bit indices)
            indexCount = indexDataSize / sizeof(uint32_t);
            if (indexDataSize % sizeof(uint32_t) != 0)
            {
                VH_LOG_WARN("IndexDataSize ({}) is not evenly divisible by sizeof(uint32_t), some data may be ignored", indexDataSize);
            }
        }

        VH_LOG_INFO("Created Mesh with {} vertices and {} indices", vertexCount, indexCount);

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(device, Move(vertexBuffer), Move(indexBuffer), vertexSize, std::move(vertexAttributesVec))));
    }

    Mesh::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_VertexBuffer(Move(other.m_VertexBuffer))
        , m_IndexBuffer(Move(other.m_IndexBuffer))
        , m_VertexSize(other.m_VertexSize)
        , m_VertexAttributes(Move(other.m_VertexAttributes))
    {
        other.m_Device = nullptr;
    }

    Mesh::Impl& Mesh::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        VH_LOG_DEBUG("Destroying Mesh Implementation");

        m_Device = other.m_Device;
        m_VertexBuffer = Move(other.m_VertexBuffer);
        m_IndexBuffer = Move(other.m_IndexBuffer);
        m_VertexSize = other.m_VertexSize;
        m_VertexAttributes = Move(other.m_VertexAttributes);

        other.m_Device = nullptr;

        return *this;
    }

    Mesh::Impl::~Impl()
    {
        VH_LOG_DEBUG("Destroying Mesh Implementation");
    }

    void Mesh::Impl::Bind(const SharedPtr<CommandBuffer::Impl> commandBuffer) const
    {
        VkCommandBuffer vkCommandBuffer = commandBuffer->GetCommandBuffer();

        // Bind vertex buffer
        SharedPtr<Buffer::Impl> vertexBufferImpl = Buffer::Impl::GetImplementation(m_VertexBuffer);
        VkBuffer vertexBuffer = vertexBufferImpl->GetBuffer();
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, &vertexBuffer, offsets);

        // Bind index buffer if it exists
        if (m_IndexBuffer.Get() != nullptr)
        {
            SharedPtr<Buffer::Impl> indexBufferImpl = Buffer::Impl::GetImplementation(*m_IndexBuffer);
            VkBuffer indexBuffer = indexBufferImpl->GetBuffer();
            // Using 32-bit indices (VK_INDEX_TYPE_UINT32)
            vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void Mesh::Impl::Draw(const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t instanceCount, uint32_t firstInstance) const
    {
        VkCommandBuffer vkCommandBuffer = commandBuffer->GetCommandBuffer();

        if (m_IndexBuffer.Get() != nullptr)
        {
            // Draw indexed
            uint32_t indexCount = m_IndexBuffer->GetSize() / sizeof(uint32_t);
            vkCmdDrawIndexed(vkCommandBuffer, indexCount, instanceCount, 0, 0, firstInstance);
        }
        else
        {
            // Draw non-indexed
            uint32_t vertexCount = m_VertexBuffer.GetSize() / m_VertexSize;
            vkCmdDraw(vkCommandBuffer, vertexCount, instanceCount, 0, firstInstance);
        }
    }

    Mesh::VertexBindingDescription Mesh::Impl::GetBindingDescription() const
    {
        VertexBindingDescription bindingDesc;
        bindingDesc.Binding = 0; // Assuming single binding for simplicity
        bindingDesc.Stride = m_VertexSize;
        bindingDesc.InputRate = Mesh::InputRate::VERTEX; // Assuming per-vertex data

        return bindingDesc;
    }

    VulkanHelper::Vector<Mesh::VertexAttributeDescription> Mesh::Impl::CreateAttributeDescriptions(const VulkanHelper::Format* formats, uint32_t count)
    {
        VulkanHelper::Vector<Mesh::VertexAttributeDescription> attributes;
        attributes.Reserve(count);

        uint32_t vertexSize = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t formatSize = VulkanHelper::GetFormatSize(formats[i]);
            Mesh::VertexAttributeDescription attr;
            attr.Location = i;
            attr.Binding = 0; // Assuming single binding for simplicity
            attr.Format = formats[i];
            attr.Offset = vertexSize;
            vertexSize += formatSize;

            attributes.PushBack(attr);
        }

        return attributes;
    }

    Mesh::VertexBindingDescription Mesh::Impl::CreateBindingDescription(uint32_t vertexSize)
    {
        VertexBindingDescription bindingDesc;
        bindingDesc.Binding = 0; // Assuming single binding for simplicity
        bindingDesc.Stride = vertexSize;
        bindingDesc.InputRate = Mesh::InputRate::VERTEX;
        return bindingDesc;
    }
    
    //
    //  Forward Functions
    //

    Expected<Mesh, VHResult> Mesh::New(const Config& config)
    {
        VH_LOG_INFO("Creating Mesh Implementation");

        if (!config.CommandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            CommandBuffer::Impl::GetImplementation(*config.CommandBuffer),
            config.VertexAttributes,
            config.VertexAttributeCount,
            config.VertexData,
            config.VertexDataSize,
            config.IndexData,
            config.IndexDataSize,
            config.AdditionalUsageFlags
        );

        if (!implResult.HasValue())
        {
            return Unexpected(implResult.Error());
        }

        return Mesh::Impl::CreatePublicInterface(Move(implResult.Value()));
    }

    //
    //  Forward Functions
    //

    Mesh::Mesh()
        : m_Impl(nullptr)
    {
    }

    Mesh::Mesh(const Mesh& other)
        : m_Impl(other.m_Impl)
    {
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = Move(other.m_Impl);

        return *this;
    }

    Mesh& Mesh::operator=(const Mesh& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;

        return *this;
    }

    Mesh::~Mesh()
    {

    }

    Mesh::Mesh(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
    }

    void Mesh::Bind(CommandBuffer& commandBuffer) const
    {
        m_Impl->Bind(CommandBuffer::Impl::GetImplementation(commandBuffer));
    }

    void Mesh::Draw(CommandBuffer& commandBuffer, uint32_t instanceCount, uint32_t firstInstance) const
    {
        m_Impl->Draw(CommandBuffer::Impl::GetImplementation(commandBuffer), instanceCount, firstInstance);
    }

    const VulkanHelper::Vector<Mesh::VertexAttributeDescription>* Mesh::GetAttributesDescriptions() const
    {
        return m_Impl->GetAttributesDescriptions();
    }

    Mesh::VertexBindingDescription Mesh::GetBindingDescription() const
    {
        return m_Impl->GetBindingDescription();
    }

    VulkanHelper::Vector<Mesh::VertexAttributeDescription> Mesh::CreateAttributeDescriptions(const VulkanHelper::Format* formats, uint32_t count)
    {
        return Impl::CreateAttributeDescriptions(formats, count);
    }

    Mesh::VertexBindingDescription Mesh::CreateBindingDescription(uint32_t vertexSize)
    {
        return Impl::CreateBindingDescription(vertexSize);
    }

    Buffer Mesh::GetVertexBuffer() const
    {
        return m_Impl->GetVertexBuffer();
    }

    Buffer Mesh::GetIndexBuffer() const
    {
        return m_Impl->GetIndexBuffer();
    }

    uint32_t Mesh::GetVertexSize() const
    {
        return m_Impl->GetVertexSize();
    }
}
