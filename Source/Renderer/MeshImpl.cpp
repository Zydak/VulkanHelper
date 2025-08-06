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
    Expected<UniquePtr<Mesh::Impl>, VHResult> Mesh::Impl::New(Device::Impl* device, CommandBuffer* commandBuffer, Format* vertexAttributes, uint32_t vertexAttributeCount, void* vertexData, uint32_t vertexDataSize, void* indexData, uint32_t indexDataSize)
    {
        if (!vertexAttributes || vertexAttributeCount == 0)
        {
            VH_LOG_ERROR("VertexAttributes cannot be null and VertexAttributeCount must be greater than 0");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (!vertexData || vertexDataSize == 0)
        {
            VH_LOG_ERROR("VertexData cannot be null and VertexDataSize must be greater than 0");
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
            Buffer::Usage::VERTEX_BUFFER | Buffer::Usage::TRANSFER_DST,
            false, // CpuMapable
            false, // UsePersistentStagingBuffer
            "Mesh Vertex Buffer"
        );
        if (!vertexBufferResult.HasValue())
        {
            VH_LOG_ERROR("Failed to create vertex buffer");
            return Unexpected(vertexBufferResult.Error());
        }

        Buffer vertexBuffer = Move(Buffer::Impl::CreatePublicInterface(Move(vertexBufferResult.Value())));

        // Upload vertex data
        VHResult uploadResult = vertexBuffer.UploadData(vertexData, vertexDataSize, 0, commandBuffer);
        if (uploadResult != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to upload vertex data to buffer");
            return Unexpected(uploadResult);
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
                Buffer::Usage::INDEX_BUFFER | Buffer::Usage::TRANSFER_DST,
                false, // CpuMapable
                false, // UsePersistentStagingBuffer
                "Mesh Index Buffer"
            );
            if (!indexBufferResult.HasValue())
            {
                VH_LOG_ERROR("Failed to create index buffer");
                return Unexpected(indexBufferResult.Error());
            }

            indexBuffer = UniquePtr<Buffer>(new Buffer(Move(Buffer::Impl::CreatePublicInterface(Move(indexBufferResult.Value())))));

            // Upload index data
            uploadResult = indexBuffer->UploadData(indexData, indexDataSize, 0, commandBuffer);
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

        return UniquePtr<Impl>(new Impl(device, Move(vertexBuffer), Move(indexBuffer), vertexSize, std::move(vertexAttributesVec)));
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

        this->~Impl(); // Clean up current state

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

    void Mesh::Impl::Bind(CommandBuffer* commandBuffer) const
    {
        if (!commandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null");
            return;
        }

        CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(commandBuffer);
        VkCommandBuffer vkCommandBuffer = cmdImpl->GetCommandBuffer();

        // Bind vertex buffer
        Buffer::Impl* vertexBufferImpl = Buffer::Impl::GetImplementation(&m_VertexBuffer);
        VkBuffer vertexBuffer = vertexBufferImpl->GetBuffer();
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, &vertexBuffer, offsets);

        // Bind index buffer if it exists
        if (m_IndexBuffer.Get() != nullptr)
        {
            Buffer::Impl* indexBufferImpl = Buffer::Impl::GetImplementation(m_IndexBuffer.Get());
            VkBuffer indexBuffer = indexBufferImpl->GetBuffer();
            // Using 32-bit indices (VK_INDEX_TYPE_UINT32)
            vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
    }

    void Mesh::Impl::Draw(CommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t firstInstance) const
    {
        if (!commandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null");
            return;
        }

        CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(commandBuffer);
        VkCommandBuffer vkCommandBuffer = cmdImpl->GetCommandBuffer();

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

    const VulkanHelper::Vector<Mesh::VertexAttributeDescription>& Mesh::Impl::GetAttributesDescriptions() const
    {
        return m_VertexAttributes;
    }

    Mesh::VertexBindingDescription Mesh::Impl::GetBindingDescription() const
    {
        VertexBindingDescription bindingDesc;
        bindingDesc.Binding = 0; // Assuming single binding for simplicity
        bindingDesc.Stride = m_VertexSize;
        bindingDesc.PerInstance = Mesh::InputRate::VERTEX; // Assuming per-vertex data

        return bindingDesc;
    }
    
    //
    //  Forward Functions
    //

    Expected<Mesh, VHResult> Mesh::New(const Config& config)
    {
        VH_LOG_INFO("Creating Mesh Implementation");

        if (!config.Device)
        {
            VH_LOG_ERROR("Device cannot be null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.CommandBuffer,
            config.VertexAttributes,
            config.VertexAttributeCount,
            config.VertexData,
            config.VertexDataSize,
            config.IndexData,
            config.IndexDataSize
        );

        if (!implResult.HasValue())
        {
            return Unexpected(implResult.Error());
        }

        return Mesh{ Move(implResult.Value()) };
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Mesh(); // Clean up current state

        m_Impl = Move(other.m_Impl);

        return *this;
    }

    Mesh::~Mesh()
    {
    }

    Mesh::Mesh(UniquePtr<Impl>&& impl)
        : m_Impl(Move(impl))
    {
    }

    void Mesh::Bind(CommandBuffer* commandBuffer) const
    {
        m_Impl->Bind(commandBuffer);
    }

    void Mesh::Draw(CommandBuffer* commandBuffer, uint32_t instanceCount, uint32_t firstInstance) const
    {
        m_Impl->Draw(commandBuffer, instanceCount, firstInstance);
    }

    const VulkanHelper::Vector<Mesh::VertexAttributeDescription>& Mesh::GetAttributesDescriptions() const
    {
        return m_Impl->GetAttributesDescriptions();
    }

    Mesh::VertexBindingDescription Mesh::GetBindingDescription() const
    {
        return m_Impl->GetBindingDescription();
    }
}
