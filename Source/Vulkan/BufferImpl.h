#pragma once

#include "Core/Enums.h"
#include "Vulkan/Buffer.h"
#include "DeviceImpl.h"
#include "ImageImpl.h"
#include "CommandBufferImpl.h"

namespace VulkanHelper
{
    class Buffer::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            SharedPtr<Device::Impl> device,
            uint64_t size,
            Buffer::Usage usage,
            bool cpuMapable,
            bool usePersistentStagingBuffer,
            uint32_t minAlignment,
            const char* debugName
        );

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Buffer& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Buffer CreatePublicInterface(const SharedPtr<Impl>& impl) { return Buffer(impl); }

        [[nodiscard]] inline SharedPtr<Device::Impl> GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkBuffer GetBuffer() const { return m_BufferAllocation.Buffer; }
        [[nodiscard]] inline uint64_t GetSize() const { return m_Size; }
        [[nodiscard]] inline Usage GetUsage() const { return m_Usage; }
        [[nodiscard]] inline bool IsMapped() const { return m_MappedData != nullptr; }

        [[nodiscard]] VkDeviceAddress GetDeviceAddress() const;

        [[nodiscard]] VHResult UploadData(const void* data, uint64_t size, uint64_t offset, SharedPtr<CommandBuffer::Impl> cmd = nullptr);
        [[nodiscard]] VHResult DownloadData(void* data, uint64_t size, uint64_t offset, SharedPtr<CommandBuffer::Impl> cmd = nullptr) const;

        [[nodiscard]] Expected<void*, VHResult> Map();

        void Unmap();

        [[nodiscard]] VHResult CopyFrom(SharedPtr<CommandBuffer::Impl> cmd, const Buffer::Impl& source, uint64_t srcOffset, uint64_t dstOffset, uint64_t size);

        [[nodiscard]] VHResult CopyToImage(SharedPtr<CommandBuffer::Impl> cmd, const SharedPtr<Image::Impl>& dst, uint32_t bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight);

        void Barrier(SharedPtr<CommandBuffer::Impl> cmd, AccessFlags srcAccess, AccessFlags dstAccess, PipelineStages srcStage, PipelineStages dstStage);
    private:
        SharedPtr<Device::Impl> m_Device;
        VulkanMemoryAllocator::BufferAllocation m_BufferAllocation;
        uint64_t m_Size;
        Usage m_Usage;
        bool m_Mapable;
        void* m_MappedData;
        
        // Optional persistent scratch buffer for non-mappable buffers
        VulkanMemoryAllocator::BufferAllocation m_ScratchBuffer;

        Impl(
            SharedPtr<Device::Impl> device,
            VulkanMemoryAllocator::BufferAllocation bufferAllocation,
            uint64_t size,
            Usage usage,
            bool mapable,
            VulkanMemoryAllocator::BufferAllocation scratchBuffer)
            : m_Device(device)
            , m_BufferAllocation(bufferAllocation)
            , m_Size(size)
            , m_Usage(usage)
            , m_Mapable(mapable)
            , m_MappedData(nullptr)
            , m_ScratchBuffer(scratchBuffer)
        {}

        /**
         * @brief Create a temporary staging buffer for a single operation.
         *
         * @param size Size of the staging buffer in bytes.
         * @param usage Usage flags for the staging buffer.
         * @return Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> Staging buffer allocation on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> CreateTemporaryStagingBuffer(uint64_t size, Usage usage) const;
    };
}