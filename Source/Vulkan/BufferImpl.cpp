#include "BufferImpl.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <cstring>

#include "Core/Move.h"
#include "Vulkan/CommandBuffer.h"
#include "CommandBufferImpl.h"
#include "Vulkan/Image.h"
#include "ImageImpl.h"
#include "Vulkan/PhysicalDevice.h"
#include "PhysicalDeviceImpl.h"
#include "Log/Log.h"

namespace VulkanHelper
{
    Expected<UniquePtr<Buffer::Impl>, VHResult> Buffer::Impl::New(const Buffer::Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Buffer Implementation");

        if (!config.Device)
        {
            VH_LOG_ERROR("Device cannot be null!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.Size == 0)
        {
            VH_LOG_ERROR("Buffer size cannot be zero!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        Device::Impl* deviceImpl = Device::Impl::GetImplementation(config.Device);

        // Create buffer info
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = config.Size;
        bufferInfo.usage = static_cast<VkBufferUsageFlags>(config.Usage);
        
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Allocate buffer using device allocator
        auto allocationResult = deviceImpl->AllocateBuffer(bufferInfo, config.CpuMapable);
        if (!allocationResult.HasValue())
        {
            VH_LOG_ERROR("Failed to allocate buffer using VulkanMemoryAllocator");
            return Unexpected(allocationResult.Error());
        }

        VulkanMemoryAllocator::BufferAllocation bufferAllocation = allocationResult.Value();

        // Set debug name if provided
        if (config.DebugName && strlen(config.DebugName) > 0)
        {
            VkDevice device = deviceImpl->GetDevice();
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(bufferAllocation.Buffer);
            nameInfo.pObjectName = config.DebugName;
            
            // TODO: do this on the device not here
            auto vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));
            if (vkSetDebugUtilsObjectNameEXT)
            {
                vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
            }
        }

        // Create optional persistent scratch buffer
        VulkanMemoryAllocator::BufferAllocation scratchBuffer = {};
        if (!config.CpuMapable && config.UsePersistentStagingBuffer)
        {
            VH_LOG_INFO("Creating persistent scratch buffer for buffer with size {} bytes", config.Size);
            
            VkBufferCreateInfo scratchBufferInfo{};
            scratchBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            scratchBufferInfo.size = config.Size;
            scratchBufferInfo.usage = static_cast<VkBufferUsageFlags>(Usage::TRANSFER_SRC | Usage::TRANSFER_DST);
            scratchBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            auto scratchAllocationResult = deviceImpl->AllocateBuffer(scratchBufferInfo, true); // Always CPU mappable
            if (!scratchAllocationResult.HasValue())
            {
                VH_LOG_ERROR("Failed to allocate scratch buffer using VulkanMemoryAllocator");
                deviceImpl->DeallocateBuffer(bufferAllocation); // Clean up main buffer
                return Unexpected(scratchAllocationResult.Error());
            }

            scratchBuffer = scratchAllocationResult.Value();
        }

        auto impl = UniquePtr<Impl>(new Impl(
            deviceImpl,
            bufferAllocation,
            config.Size,
            config.Usage,
            config.CpuMapable,
            scratchBuffer
        ));
        
        return impl;
    }

    Buffer::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_BufferAllocation(other.m_BufferAllocation)
        , m_Size(other.m_Size)
        , m_Usage(other.m_Usage)
        , m_Mapable(other.m_Mapable)
        , m_MappedData(other.m_MappedData)
        , m_ScratchBuffer(other.m_ScratchBuffer)
    {
        other.m_Device = nullptr;
        other.m_BufferAllocation = {};
        other.m_Size = 0;
        other.m_MappedData = nullptr;
        other.m_ScratchBuffer = {};
    }

    Buffer::Impl& Buffer::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        // Clean up current resources
        this->~Impl();

        // Move from other
        m_Device = other.m_Device;
        m_BufferAllocation = other.m_BufferAllocation;
        m_Size = other.m_Size;
        m_Usage = other.m_Usage;
        m_Mapable = other.m_Mapable;
        m_MappedData = other.m_MappedData;
        m_ScratchBuffer = other.m_ScratchBuffer;

        // Reset other
        other.m_Device = nullptr;
        other.m_BufferAllocation = {};
        other.m_Size = 0;
        other.m_MappedData = nullptr;
        other.m_ScratchBuffer = {};

        return *this;
    }

    Buffer::Impl::~Impl()
    {
        if (m_Device)
        {
            // VMA will automatically handle unmapping when the allocation is destroyed
            if (m_MappedData)
            {
                m_MappedData = nullptr;
            }

            // Deallocate scratch buffer first if it exists
            if (m_ScratchBuffer.Buffer != VK_NULL_HANDLE)
            {
                m_Device->DeallocateBuffer(m_ScratchBuffer);
            }

            // Deallocate main buffer
            if (m_BufferAllocation.Buffer != VK_NULL_HANDLE)
            {
                m_Device->DeallocateBuffer(m_BufferAllocation);
            }
        }
    }

    VHResult Buffer::Impl::UploadData(const void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd)
    {
        if (!data)
        {
            VH_LOG_ERROR("Data pointer cannot be null!");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (offset + size > m_Size)
        {
            VH_LOG_ERROR("Upload size {} + offset {} exceeds buffer size {}", size, offset, m_Size);
            return VHResult::WRONG_ARGUMENTS;
        }

        // For CPU-accessible memory, map and copy directly
        if (m_Mapable)
        {
            auto mapResult = m_Device->MapBuffer(m_BufferAllocation);
            if (!mapResult.HasValue())
            {
                VH_LOG_ERROR("Failed to map buffer memory for upload using VMA");
                return mapResult.Error();
            }

            void* mappedData = mapResult.Value();
            memcpy(static_cast<char*>(mappedData) + offset, data, size);
            m_Device->UnmapBuffer(m_BufferAllocation);
        }
        else
        {
            if (!cmd)
            {
                VH_LOG_ERROR("CommandBuffer is required for uploading to non-mappable buffers");
                return VHResult::WRONG_ARGUMENTS;
            }

            // For non-mappable buffers, use scratch buffer
            VulkanMemoryAllocator::BufferAllocation scratchAllocation;
            bool useTemporaryStaging = false;

            const bool hasScratchBuffer = m_ScratchBuffer.Buffer != VK_NULL_HANDLE;
            if (hasScratchBuffer)
            {
                // Use persistent scratch buffer
                scratchAllocation = m_ScratchBuffer;
            }
            else
            {
                // Create temporary scratch buffer
                auto tempScratchResult = CreateTemporaryStagingBuffer(size, Usage::TRANSFER_SRC);
                if (!tempScratchResult.HasValue())
                {
                    VH_LOG_ERROR("Failed to create temporary scratch buffer for upload");
                    return tempScratchResult.Error();
                }
                scratchAllocation = tempScratchResult.Value();
                useTemporaryStaging = true;
                VH_LOG_DEBUG("Created temporary scratch buffer for upload");
            }

            // Map scratch buffer and copy data
            auto mapResult = m_Device->MapBuffer(scratchAllocation);
            if (!mapResult.HasValue())
            {
                VH_LOG_ERROR("Failed to map scratch buffer for upload");
                if (useTemporaryStaging)
                    m_Device->DeallocateBuffer(scratchAllocation);
                return mapResult.Error();
            }

            void* scratchMappedData = mapResult.Value();
            uint64_t scratchOffset = hasScratchBuffer ? offset : 0;
            memcpy(static_cast<char*>(scratchMappedData) + scratchOffset, data, size);
            m_Device->UnmapBuffer(scratchAllocation);

            // Copy from scratch buffer to main buffer using GPU
            CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(cmd);
            VkCommandBuffer commandBuffer = cmdImpl->GetCommandBuffer();

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = scratchOffset;
            copyRegion.dstOffset = offset;
            copyRegion.size = size;

            vkCmdCopyBuffer(commandBuffer, scratchAllocation.Buffer, m_BufferAllocation.Buffer, 1, &copyRegion);
            
            if (useTemporaryStaging)
            {
                // Unfortunately if we're using temporary stagin buffer, the copy command has to be exectured BEFORE the buffer is dealocated.
                // Which means the cmd has to end, submit and start again before the scratch can be dealocated.
                VHResult res = cmd->EndRecording();
                if (res != VHResult::OK)
                {
                    VH_LOG_ERROR("Couldn't end recording of the command buffer! Make sure it is in recording state!");
                    m_Device->DeallocateBuffer(scratchAllocation);
                    return res;
                }
                res = cmd->SubmitAndWait();
                if (res != VHResult::OK)
                {
                    VH_LOG_ERROR("Couldn't end Submit the command buffer!");
                    m_Device->DeallocateBuffer(scratchAllocation);
                    return res;
                }
                res = cmd->BeginRecording(VulkanHelper::CommandBuffer::Usage::NONE);
                if (res != VHResult::OK)
                {
                    VH_LOG_ERROR("Couldn't begin CommandBuffer recording!");
                    m_Device->DeallocateBuffer(scratchAllocation);
                    return res;
                }

                m_Device->DeallocateBuffer(scratchAllocation);
            }
        }

        return VHResult::OK;
    }

    VHResult Buffer::Impl::DownloadData(void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd) const
    {
        if (!data)
        {
            VH_LOG_ERROR("Data pointer cannot be null!");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (offset + size > m_Size)
        {
            VH_LOG_ERROR("Download size {} + offset {} exceeds buffer size {}", size, offset, m_Size);
            return VHResult::WRONG_ARGUMENTS;
        }

        // For CPU-accessible memory, map and copy directly
        if (m_Mapable)
        {
            auto mapResult = m_Device->MapBuffer(m_BufferAllocation);
            if (!mapResult.HasValue())
            {
                VH_LOG_ERROR("Failed to map buffer memory for download using VMA");
                return mapResult.Error();
            }

            void* mappedData = mapResult.Value();
            memcpy(data, static_cast<const char*>(mappedData) + offset, size);
            m_Device->UnmapBuffer(m_BufferAllocation);

            return VHResult::OK;
        }
        else
        {
            if (!cmd)
            {
                VH_LOG_ERROR("CommandBuffer is required for downloading from non-mappable buffers");
                return VHResult::WRONG_ARGUMENTS;
            }

            // For non-mappable buffers, use scratch buffer
            VulkanMemoryAllocator::BufferAllocation scratchAllocation;
            bool useTemporaryStaging = false;

            const bool hasScratchBuffer = m_ScratchBuffer.Buffer != VK_NULL_HANDLE;
            if (hasScratchBuffer)
            {
                // Use persistent scratch buffer
                scratchAllocation = m_ScratchBuffer;
            }
            else
            {
                // Create temporary scratch buffer
                auto tempScratchResult = CreateTemporaryStagingBuffer(size, Usage::TRANSFER_DST);
                if (!tempScratchResult.HasValue())
                {
                    VH_LOG_ERROR("Failed to create temporary scratch buffer for download");
                    return tempScratchResult.Error();
                }
                scratchAllocation = tempScratchResult.Value();
                useTemporaryStaging = true;
                VH_LOG_DEBUG("Created temporary scratch buffer for download");
            }

            // Copy from main buffer to scratch buffer using GPU
            uint64_t scratchOffset = hasScratchBuffer ? offset : 0;
            
            CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(cmd);
            VkCommandBuffer commandBuffer = cmdImpl->GetCommandBuffer();

            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = offset;
            copyRegion.dstOffset = scratchOffset;
            copyRegion.size = size;

            vkCmdCopyBuffer(commandBuffer, m_BufferAllocation.Buffer, scratchAllocation.Buffer, 1, &copyRegion);
            VHResult copyResult = VHResult::OK;
            
            if (copyResult == VHResult::OK)
            {
                // Map scratch buffer and copy data to CPU memory
                auto mapResult = m_Device->MapBuffer(scratchAllocation);
                if (!mapResult.HasValue())
                {
                    VH_LOG_ERROR("Failed to map scratch buffer for download");
                    // Error occurred during mapping
                    if (useTemporaryStaging)
                        m_Device->DeallocateBuffer(scratchAllocation);
                    return mapResult.Error();
                }

                void* scratchMappedData = mapResult.Value();
                memcpy(data, static_cast<const char*>(scratchMappedData) + scratchOffset, size);
                m_Device->UnmapBuffer(scratchAllocation);
            }

            // GPU copy completed
            
            if (useTemporaryStaging)
            {
                // Unfortunately if we're using temporary stagin buffer, the copy command has to be exectured BEFORE the buffer is dealocated.
                // Which means the cmd has to end, submit and start again before the scratch can be dealocated.
                VHResult res = cmd->EndRecording();
                if (res != VHResult::OK)
                {
                    VH_LOG_ERROR("Couldn't end recording of the command buffer! Make sure it is in recording state!");
                    m_Device->DeallocateBuffer(scratchAllocation);
                    return res;
                }
                res = cmd->SubmitAndWait();
                if (res != VHResult::OK)
                {
                    VH_LOG_ERROR("Couldn't end Submit the command buffer!");
                    m_Device->DeallocateBuffer(scratchAllocation);
                    return res;
                }
                res = cmd->BeginRecording(VulkanHelper::CommandBuffer::Usage::NONE);
                if (res != VHResult::OK)
                {
                    VH_LOG_ERROR("Couldn't begin CommandBuffer recording!");
                    m_Device->DeallocateBuffer(scratchAllocation);
                    return res;
                }

                m_Device->DeallocateBuffer(scratchAllocation);
            }

            return copyResult;
        }
    }

    Expected<void*, VHResult> Buffer::Impl::Map()
    {
        if (m_MappedData)
        {
            VH_LOG_WARN("Buffer is already mapped!");
            return m_MappedData;
        }

        if (!m_Mapable)
        {
            VH_LOG_ERROR("Cannot map GPU-only buffer memory!");
            return Unexpected(VHResult::MEMORY_MAP_FAILED);
        }

        auto mapResult = m_Device->MapBuffer(m_BufferAllocation);
        if (!mapResult.HasValue())
        {
            VH_LOG_ERROR("Failed to map buffer memory using VMA");
            return Unexpected(mapResult.Error());
        }

        m_MappedData = mapResult.Value();
        return m_MappedData;
    }

    void Buffer::Impl::Unmap()
    {
        if (!m_MappedData)
        {
            VH_LOG_WARN("Buffer is not currently mapped!");
            return;
        }

        m_Device->UnmapBuffer(m_BufferAllocation);
        m_MappedData = nullptr;
    }

    VHResult Buffer::Impl::CopyFrom(CommandBuffer& cmd, const Buffer& source, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(&cmd);
        VkCommandBuffer commandBuffer = cmdImpl->GetCommandBuffer();

        Buffer::Impl* sourceImpl = Buffer::Impl::GetImplementation(&source);
        VkBuffer srcBuffer = sourceImpl->GetBuffer();

        // If size is UINT64_MAX (equivalent to VK_WHOLE_SIZE), use the minimum of the two buffer sizes
        if (size == UINT64_MAX)
        {
            size = (sourceImpl->GetSize() < m_Size) ? sourceImpl->GetSize() : m_Size;
            size -= (srcOffset > dstOffset) ? srcOffset : dstOffset;
        }

        if (srcOffset + size > sourceImpl->GetSize())
        {
            VH_LOG_ERROR("Source copy range exceeds source buffer size");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (dstOffset + size > m_Size)
        {
            VH_LOG_ERROR("Destination copy range exceeds destination buffer size");
            return VHResult::WRONG_ARGUMENTS;
        }

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, srcBuffer, m_BufferAllocation.Buffer, 1, &copyRegion);

        return VHResult::OK;
    }

    VHResult Buffer::Impl::CopyToImage(CommandBuffer& cmd, const Image& dst, uint32_t bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight)
    {
        CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(&cmd);
        VkCommandBuffer commandBuffer = cmdImpl->GetCommandBuffer();

        Image::Impl* imageImpl = Image::Impl::GetImplementation(&dst);
        VkImage dstImage = imageImpl->GetImage();

        VkBufferImageCopy region{};
        region.bufferOffset = bufferOffset;
        region.bufferRowLength = bufferRowLength;
        region.bufferImageHeight = bufferImageHeight;
        
        region.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(imageImpl->GetAspect());
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = imageImpl->GetLayerCount();
        
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            imageImpl->GetWidth(),
            imageImpl->GetHeight(),
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, m_BufferAllocation.Buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        return VHResult::OK;
    }

    VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> 
    Buffer::Impl::CreateTemporaryStagingBuffer(uint64_t size, Usage usage) const
    {
        VkBufferCreateInfo stagingBufferInfo{};
        stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingBufferInfo.size = size;
        stagingBufferInfo.usage = static_cast<VkBufferUsageFlags>(usage);
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return m_Device->AllocateBuffer(stagingBufferInfo, true); // Always CPU mappable
    }

    // Public Buffer class implementations
    Expected<Buffer, VHResult> Buffer::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return Unexpected(implResult.Error());
        }

        return Buffer{ Move(implResult.Value()) };
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {}

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Buffer(); // Clean up current state
        m_Impl = Move(other.m_Impl);
        return *this;
    }

    Buffer::~Buffer()
    {
        // Impl destructor will handle cleanup
    }

    Buffer::Buffer(UniquePtr<Impl>&& impl)
        : m_Impl(Move(impl))
    {}

    VHResult Buffer::UploadData(const void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd)
    {
        return m_Impl->UploadData(data, size, offset, cmd);
    }

    VHResult Buffer::DownloadData(void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd) const
    {
        return m_Impl->DownloadData(data, size, offset, cmd);
    }

    Expected<void*, VHResult> Buffer::Map()
    {
        return m_Impl->Map();
    }

    void Buffer::Unmap()
    {
        m_Impl->Unmap();
    }

    VHResult Buffer::CopyFrom(CommandBuffer& cmd, const Buffer& source, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        return m_Impl->CopyFrom(cmd, source, srcOffset, dstOffset, size);
    }

    VHResult Buffer::CopyToImage(CommandBuffer& cmd, const Image& dst, uint32_t bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight)
    {
        return m_Impl->CopyToImage(cmd, dst, bufferOffset, bufferRowLength, bufferImageHeight);
    }

    uint64_t Buffer::GetSize() const
    {
        return m_Impl->GetSize();
    }

    Buffer::Usage Buffer::GetUsage() const
    {
        return m_Impl->GetUsage();
    }

    bool Buffer::IsMapped() const
    {
        return m_Impl->IsMapped();
    }
}