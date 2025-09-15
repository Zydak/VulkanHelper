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
    Expected<SharedPtr<Buffer::Impl>, VHResult> Buffer::Impl::New(
        SharedPtr<Device::Impl> device,
        uint64_t size,
        Buffer::Usage usage,
        bool cpuMapable,
        uint32_t minAlignment,
        const char* debugName,
        uint32_t deleteDelayFrames
    )
    {
        VH_LOG_INFO("Creating Vulkan Buffer Implementation");

        if (!device)
        {
            VH_LOG_ERROR("Device cannot be null!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (size == 0)
        {
            VH_LOG_ERROR("Buffer size cannot be zero!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Create buffer info
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = static_cast<VkBufferUsageFlags>(usage);
        
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // Allocate buffer using device allocator
        auto allocationResult = device->AllocateBuffer(bufferInfo, cpuMapable, minAlignment);
        if (!allocationResult.HasValue())
        {
            VH_LOG_ERROR("Failed to allocate buffer using VulkanMemoryAllocator");
            return Unexpected(allocationResult.Error());
        }

        VulkanMemoryAllocator::BufferAllocation bufferAllocation = allocationResult.Value();

        // Set debug name if provided
        if (debugName && strlen(debugName) > 0)
        {
            VkDevice deviceHandle = device->GetDevice();
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            nameInfo.objectHandle = (uint64_t)bufferAllocation.Buffer;
            nameInfo.pObjectName = debugName;
            
            // TODO: do this on the device not here
            auto vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
                vkGetDeviceProcAddr(deviceHandle, "vkSetDebugUtilsObjectNameEXT"));
            if (vkSetDebugUtilsObjectNameEXT)
            {
                vkSetDebugUtilsObjectNameEXT(deviceHandle, &nameInfo);
            }
        }

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(
            device,
            bufferAllocation,
            size,
            usage,
            deleteDelayFrames,
            cpuMapable
        )));
    }

    Buffer::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_BufferAllocation(other.m_BufferAllocation)
        , m_Size(other.m_Size)
        , m_Usage(other.m_Usage)
        , m_DeleteDelayFrames(other.m_DeleteDelayFrames)
        , m_MappedData(other.m_MappedData)
        , m_Mapable(other.m_Mapable)
    {
        other.m_Device = nullptr;
        other.m_BufferAllocation = {};
        other.m_Size = 0;
        other.m_MappedData = nullptr;
        other.m_DeleteDelayFrames = 0;
    }

    Buffer::Impl& Buffer::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (m_Device)
        {
            // Deallocate buffer
            if (m_BufferAllocation.Buffer != VK_NULL_HANDLE)
            {
                m_Device->GetDeleteQueue().QueueForDeletion(m_BufferAllocation, m_DeleteDelayFrames);
            }
        }

        // Move from other
        m_Device = other.m_Device;
        m_BufferAllocation = other.m_BufferAllocation;
        m_Size = other.m_Size;
        m_Usage = other.m_Usage;
        m_DeleteDelayFrames = other.m_DeleteDelayFrames;
        m_Mapable = other.m_Mapable;
        m_MappedData = other.m_MappedData;

        // Reset other
        other.m_Device = nullptr;
        other.m_BufferAllocation = {};
        other.m_Size = 0;
        other.m_Usage = {};
        other.m_DeleteDelayFrames = 0;
        other.m_MappedData = nullptr;

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

            if (m_BufferAllocation.Buffer != VK_NULL_HANDLE)
            {
                m_Device->GetDeleteQueue().QueueForDeletion(m_BufferAllocation, m_DeleteDelayFrames);
            }
        }
    }

    VHResult Buffer::Impl::UploadData(const void* data, uint64_t size, uint64_t offset)
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

        if (!m_Mapable)
        {
            VH_LOG_ERROR("Cannot upload data to non-mappable buffer! Use a staging buffer and copy instead.");
            return VHResult::WRONG_ARGUMENTS;
        }

        auto mapResult = m_Device->MapMemory(m_BufferAllocation.Allocation);
        if (!mapResult.HasValue())
        {
            VH_LOG_ERROR("Failed to map buffer memory for upload using VMA");
            return mapResult.Error();
        }

        void* mappedData = mapResult.Value();
        memcpy(static_cast<char*>(mappedData) + offset, data, size);
        m_Device->UnmapMemory(m_BufferAllocation.Allocation);

        return VHResult::OK;
    }

    VHResult Buffer::Impl::DownloadData(void* data, uint64_t size, uint64_t offset) const
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

        if (!m_Mapable)
        {
            VH_LOG_ERROR("Cannot download data from non-mappable buffer! Use a staging buffer and copy instead.");
            return VHResult::WRONG_ARGUMENTS;
        }

        auto mapResult = m_Device->MapMemory(m_BufferAllocation.Allocation);
        if (!mapResult.HasValue())
        {
            VH_LOG_ERROR("Failed to map buffer memory for download using VMA");
            return mapResult.Error();
        }

        void* mappedData = mapResult.Value();
        memcpy(data, static_cast<const char*>(mappedData) + offset, size);
        m_Device->UnmapMemory(m_BufferAllocation.Allocation);

        return VHResult::OK;
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

        auto mapResult = m_Device->MapMemory(m_BufferAllocation.Allocation);
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

        m_Device->UnmapMemory(m_BufferAllocation.Allocation);
        m_MappedData = nullptr;
    }

    VHResult Buffer::Impl::CopyFromBuffer(SharedPtr<CommandBuffer::Impl> cmd, const SharedPtr<Buffer::Impl>& source, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        VkCommandBuffer commandBuffer = cmd->GetCommandBuffer();

        VkBuffer srcBuffer = source->GetBuffer();

        // If size is UINT64_MAX (equivalent to VK_WHOLE_SIZE), use the minimum of the two buffer sizes
        if (size == UINT64_MAX)
        {
            size = (source->GetSize() < m_Size) ? source->GetSize() : m_Size;
            size -= (srcOffset > dstOffset) ? srcOffset : dstOffset;
        }

        if (srcOffset + size > source->GetSize())
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

    VHResult Buffer::Impl::CopyToBuffer(SharedPtr<CommandBuffer::Impl> cmd, const SharedPtr<Buffer::Impl>& destination, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        return destination->CopyFromBuffer(cmd, SharedPtr<Buffer::Impl>(this), dstOffset, srcOffset, size);
    }

    VHResult Buffer::Impl::CopyToImage(SharedPtr<CommandBuffer::Impl> cmd, const SharedPtr<Image::Impl>& dst, uint32_t bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight)
    {
        VkCommandBuffer commandBuffer = cmd->GetCommandBuffer();

        VkImage dstImage = dst->GetImage();

        VkBufferImageCopy region{};
        region.bufferOffset = bufferOffset;
        region.bufferRowLength = bufferRowLength;
        region.bufferImageHeight = bufferImageHeight;

        region.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(dst->GetAspect());
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = dst->GetLayerCount();

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            dst->GetWidth(),
            dst->GetHeight(),
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, m_BufferAllocation.Buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        return VHResult::OK;
    }

    void Buffer::Impl::Barrier(SharedPtr<CommandBuffer::Impl> cmd, AccessFlags srcAccess, AccessFlags dstAccess, PipelineStages srcStage, PipelineStages dstStage)
    {
        VkBufferMemoryBarrier bufferBarrier{};
        bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarrier.srcAccessMask = static_cast<VkAccessFlags>(srcAccess);
        bufferBarrier.dstAccessMask = static_cast<VkAccessFlags>(dstAccess);
        bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufferBarrier.buffer = m_BufferAllocation.Buffer;
        bufferBarrier.offset = 0;
        bufferBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(
            cmd->GetCommandBuffer(),
            static_cast<VkPipelineStageFlags>(srcStage),
            static_cast<VkPipelineStageFlags>(dstStage),
            0,
            0, nullptr,
            1, &bufferBarrier,
            0, nullptr
        );
    }

    [[nodiscard]] VkDeviceAddress Buffer::Impl::GetDeviceAddress() const
    {
        VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
        bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bufferDeviceAddressInfo.buffer = m_BufferAllocation.Buffer;

        return vkGetBufferDeviceAddress(m_Device->GetDevice(), &bufferDeviceAddressInfo);
    }

    [[nodiscard]] VHResult Buffer::Impl::CopyFromImage(SharedPtr<CommandBuffer::Impl> cmd, const SharedPtr<Image::Impl>& src)
    {
        VkCommandBuffer commandBuffer = cmd->GetCommandBuffer();
        VkImage srcImage = src->GetImage();

        uint64_t imageSize = src->GetSizeInBytes();
        VH_ASSERT(imageSize == m_Size, "Image and buffer must be exactly the same size, copying different sizes isn't implemented yet");

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(src->GetAspect());
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {src->GetWidth(), src->GetHeight(), 1};

        vkCmdCopyImageToBuffer(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_BufferAllocation.Buffer, 1, &copyRegion);

        return VHResult::OK;
    }

    //
    //  Forward Functions
    //

    Expected<Buffer, VHResult> Buffer::New(const Config& config)
    {
        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.Size,
            config.Usage,
            config.CpuMapable,
            config.MinAlignment,
            config.DebugName,
            1 // TODO
        );

        if (!implResult.HasValue())
        {
            return Unexpected(implResult.Error());
        }

        return Impl::CreatePublicInterface(Move(implResult.Value()));
    }

    Buffer::Buffer()
        : m_Impl(nullptr)
    {
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {}

    Buffer::Buffer(const Buffer& other)
        : m_Impl(other.m_Impl)
    {}

    Buffer& Buffer::operator=(const Buffer& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;
        return *this;
    }

    Buffer& Buffer::operator=(Buffer&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = Move(other.m_Impl);
        return *this;
    }

    Buffer::~Buffer()
    {

    }

    Buffer::Buffer(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {}

    VHResult Buffer::UploadData(const void* data, uint64_t size, uint64_t offset)
    {
        return m_Impl->UploadData(data, size, offset);
    }

    VHResult Buffer::DownloadData(void* data, uint64_t size, uint64_t offset) const
    {
        return m_Impl->DownloadData(data, size, offset);
    }

    Expected<void*, VHResult> Buffer::Map()
    {
        return m_Impl->Map();
    }

    void Buffer::Unmap()
    {
        m_Impl->Unmap();
    }

    VHResult Buffer::CopyFromBuffer(CommandBuffer& cmd, const Buffer& source, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        return m_Impl->CopyFromBuffer(CommandBuffer::Impl::GetImplementation(cmd), Buffer::Impl::GetImplementation(source), srcOffset, dstOffset, size);
    }

    VHResult Buffer::CopyToBuffer(CommandBuffer& cmd, const Buffer& destination, uint64_t srcOffset, uint64_t dstOffset, uint64_t size)
    {
        return m_Impl->CopyToBuffer(CommandBuffer::Impl::GetImplementation(cmd), Buffer::Impl::GetImplementation(destination), srcOffset, dstOffset, size);
    }

    VHResult Buffer::CopyToImage(CommandBuffer& cmd, const Image& dst, uint32_t bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight)
    {
        return m_Impl->CopyToImage(CommandBuffer::Impl::GetImplementation(cmd), Image::Impl::GetImplementation(dst), bufferOffset, bufferRowLength, bufferImageHeight);
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

    void Buffer::Barrier(CommandBuffer& cmd, AccessFlags srcAccess, AccessFlags dstAccess, PipelineStages srcStage, PipelineStages dstStage)
    {
        m_Impl->Barrier(CommandBuffer::Impl::GetImplementation(cmd), srcAccess, dstAccess, srcStage, dstStage);
    }

    VHResult Buffer::CopyFromImage(CommandBuffer& cmd, const Image& src)
    {
        return m_Impl->CopyFromImage(CommandBuffer::Impl::GetImplementation(cmd), Image::Impl::GetImplementation(src));
    }
}