#pragma once

#include "Core/Enums.h"
#include "Vulkan/Buffer.h"
#include "DeviceImpl.h"

typedef struct VkBuffer_T* VkBuffer;
typedef struct VkDeviceMemory_T* VkDeviceMemory;

namespace VulkanHelper
{
    class Buffer::Impl
    {
    public:
        /**
         * @brief Creates a new Buffer implementation instance.
         *
         * @param config Configuration parameters for buffer creation.
         * @return Expected<UniquePtr<Impl>, VHResult> Implementation instance on success or error on failure.
         */
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(const Buffer::Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        /**
         * @brief Gets the implementation instance from a public Buffer interface.
         *
         * @param publicInterface Pointer to the public Buffer instance.
         * @return Pointer to the implementation instance.
         */
        [[nodiscard]] inline static Impl* GetImplementation(const Buffer* publicInterface) 
        { 
            return publicInterface->m_Impl.Get(); 
        }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkBuffer GetBuffer() const { return m_BufferAllocation.Buffer; }
        [[nodiscard]] inline uint64_t GetSize() const { return m_Size; }
        [[nodiscard]] inline Usage GetUsage() const { return m_Usage; }
        [[nodiscard]] inline bool IsMapped() const { return m_MappedData != nullptr; }

        /**
         * @brief Upload data to the buffer.
         *
         * @param data Pointer to the data to upload.
         * @param size Size of the data in bytes.
         * @param offset Offset in the buffer to start writing.
         * @param cmd Command buffer for GPU operations (required for non-mappable buffers).
         * @return VHResult::OK on success, or an error code on failure.
         */
        VHResult UploadData(const void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd = nullptr);

        /**
         * @brief Download data from the buffer.
         *
         * @param data Pointer to the destination buffer.
         * @param size Size of the data to read in bytes.
         * @param offset Offset in the buffer to start reading.
         * @param cmd Command buffer for GPU operations (required for non-mappable buffers).
         * @return VHResult::OK on success, or an error code on failure.
         */
        VHResult DownloadData(void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd = nullptr) const;

        /**
         * @brief Map buffer memory for CPU access.
         *
         * @return Expected<void*, VHResult> Pointer to mapped memory on success, or error code on failure.
         */
        [[nodiscard]] Expected<void*, VHResult> Map();

        /**
         * @brief Unmap previously mapped buffer memory.
         */
        void Unmap();

        /**
         * @brief Copy data from another buffer using GPU commands.
         *
         * @param cmd Command buffer to record the copy operation.
         * @param source Source buffer to copy from.
         * @param srcOffset Offset in source buffer.
         * @param dstOffset Offset in destination buffer.
         * @param size Size to copy.
         * @return VHResult::OK on success, or an error code on failure.
         */
        VHResult CopyFrom(CommandBuffer& cmd, const Buffer& source, uint64_t srcOffset, uint64_t dstOffset, uint64_t size);

        /**
         * @brief Copy buffer data to an image using GPU commands.
         *
         * @param cmd Command buffer to record the copy operation.
         * @param dst Destination image.
         * @param bufferOffset Buffer offset.
         * @param bufferRowLength Buffer row length.
         * @param bufferImageHeight Buffer image height.
         * @return VHResult::OK on success, or an error code on failure.
         */
        VHResult CopyToImage(CommandBuffer& cmd, const Image& dst, uint32_t bufferOffset, uint32_t bufferRowLength, uint32_t bufferImageHeight);

    private:
        Device::Impl* m_Device;
        VulkanMemoryAllocator::BufferAllocation m_BufferAllocation;
        uint64_t m_Size;
        Usage m_Usage;
        bool m_Mapable;
        void* m_MappedData;
        
        // Optional persistent scratch buffer for non-mappable buffers
        VulkanMemoryAllocator::BufferAllocation m_ScratchBuffer;

        Impl(Device::Impl* device,
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