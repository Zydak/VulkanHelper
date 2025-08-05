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
    Expected<UniquePtr<Mesh::Impl>, VHResult> Mesh::Impl::New(const Config& config)
    {
        if (!config.Device)
        {
            VH_LOG_ERROR("Device cannot be null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (!config.VertexAttributes || config.VertexAttributeCount == 0)
        {
            VH_LOG_ERROR("VertexAttributes cannot be null and VertexAttributeCount must be greater than 0");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (!config.VertexData || config.VertexDataSize == 0)
        {
            VH_LOG_ERROR("VertexData cannot be null and VertexDataSize must be greater than 0");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (!config.CommandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        Device::Impl* deviceImpl = Device::Impl::GetImplementation(config.Device);

        // Create vertex buffer
        Buffer::Config vertexBufferConfig{};
        vertexBufferConfig.Device = config.Device;
        vertexBufferConfig.Size = config.VertexDataSize;
        vertexBufferConfig.Usage = Buffer::Usage::VERTEX_BUFFER | Buffer::Usage::TRANSFER_DST;
        vertexBufferConfig.CpuMapable = false;
        vertexBufferConfig.DebugName = "Mesh Vertex Buffer";

        auto vertexBufferResult = Buffer::New(vertexBufferConfig);
        if (!vertexBufferResult.HasValue())
        {
            VH_LOG_ERROR("Failed to create vertex buffer");
            return Unexpected(vertexBufferResult.Error());
        }

        Buffer vertexBuffer = Move(vertexBufferResult.Value());

        // Upload vertex data
        VHResult uploadResult = vertexBuffer.UploadData(config.VertexData, config.VertexDataSize, 0, config.CommandBuffer);
        if (uploadResult != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to upload vertex data to buffer");
            return Unexpected(uploadResult);
        }

        // Calculate vertex count based on vertex attribute sizes
        VulkanHelper::Vector<Mesh::VertexAttributeDescription> vertexAttributes;
        uint32_t vertexSize = 0;
        for (uint32_t i = 0; i < config.VertexAttributeCount; ++i)
        {
            Format format = config.VertexAttributes[i];
            uint32_t formatSize = VulkanHelper::GetFormatSize(format);

            Mesh::VertexAttributeDescription attr{};
            attr.Location = i;
            attr.Binding = 0; // Assuming single binding for simplicity
            attr.Format = format;
            attr.Offset = vertexSize;
            vertexAttributes.PushBack(Move(attr));

            vertexSize += formatSize;
        }

        uint32_t vertexCount = config.VertexDataSize / vertexSize;
        if (config.VertexDataSize % vertexSize != 0)
        {
            VH_LOG_ERROR("VertexDataSize ({}) is not evenly divisible by vertex size ({}), Make sure you passed the correct data!", config.VertexDataSize, vertexSize);
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Create index buffer if index data is provided
        UniquePtr<Buffer> indexBuffer(nullptr);
        uint32_t indexCount = 0;
        
        if (config.IndexData && config.IndexDataSize > 0)
        {
            Buffer::Config indexBufferConfig{};
            indexBufferConfig.Device = config.Device;
            indexBufferConfig.Size = config.IndexDataSize;
            indexBufferConfig.Usage = Buffer::Usage::INDEX_BUFFER | Buffer::Usage::TRANSFER_DST;
            indexBufferConfig.CpuMapable = false;
            indexBufferConfig.DebugName = "Mesh Index Buffer";

            auto indexBufferResult = Buffer::New(indexBufferConfig);
            if (!indexBufferResult.HasValue())
            {
                VH_LOG_ERROR("Failed to create index buffer");
                return Unexpected(indexBufferResult.Error());
            }

            indexBuffer = UniquePtr<Buffer>(new Buffer(Move(indexBufferResult.Value())));

            // Upload index data
            uploadResult = indexBuffer->UploadData(config.IndexData, config.IndexDataSize, 0, config.CommandBuffer);
            if (uploadResult != VHResult::OK)
            {
                VH_LOG_ERROR("Failed to upload index data to buffer");
                return Unexpected(uploadResult);
            }

            // Assume 32-bit indices (could be extended to support 16-bit indices)
            indexCount = config.IndexDataSize / sizeof(uint32_t);
            if (config.IndexDataSize % sizeof(uint32_t) != 0)
            {
                VH_LOG_WARN("IndexDataSize ({}) is not evenly divisible by sizeof(uint32_t), some data may be ignored", config.IndexDataSize);
            }
        }

        VH_LOG_INFO("Created Mesh with {} vertices and {} indices", vertexCount, indexCount);

        return UniquePtr<Impl>(new Impl(deviceImpl, Move(vertexBuffer), Move(indexBuffer), vertexSize, std::move(vertexAttributes)));
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
        auto implResult = Impl::New(config);
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
