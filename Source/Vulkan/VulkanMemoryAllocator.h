#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"

#include "vk_mem_alloc.h"

namespace VulkanHelper
{
    class VulkanMemoryAllocator
    {
    public:
        struct BufferAllocation
        {
            VkBuffer Buffer = VK_NULL_HANDLE;
            VmaAllocation Allocation = nullptr;
        };

        struct ImageAllocation
        {
            VkImage image = VK_NULL_HANDLE;
            VmaAllocation Allocation = nullptr;
        };

        [[nodiscard]] static VulkanHelper::Expected<VulkanMemoryAllocator, VHResult> New(VkDevice device, VkInstance instance, VkPhysicalDevice physicalDevice);

        VulkanMemoryAllocator(const VulkanMemoryAllocator& other) = delete;
        VulkanMemoryAllocator& operator=(const VulkanMemoryAllocator& other) = delete;

        VulkanMemoryAllocator(VulkanMemoryAllocator&& other) noexcept;
        VulkanMemoryAllocator& operator=(VulkanMemoryAllocator&& other) noexcept;

        ~VulkanMemoryAllocator();

        /**
         * @brief Allocate a buffer with the specified memory usage.
         *
         * @param bufferInfo Vulkan buffer creation info.
         * @param allowMapping Whether to create memory mapable or not.
         * @return Expected<BufferAllocation, VHResult> Buffer allocation on success, or error on failure.
         */
        VulkanHelper::Expected<BufferAllocation, VHResult> AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowMapping = false);

        /**
         * @brief Allocate an image with the specified memory usage.
         *
         * @param imageInfo Vulkan image creation info.
         * @param allowMapping Whether to create memory mapable or not.
         * @return Expected<ImageAllocation, VHResult> Image allocation on success, or error on failure.
         */
        VulkanHelper::Expected<ImageAllocation, VHResult> AllocateImage(const VkImageCreateInfo& imageInfo, bool allowMapping = false);

        /**
         * @brief Deallocate a previously allocated buffer.
         *
         * @param allocation Buffer allocation to deallocate.
         */
        void DeallocateBuffer(const BufferAllocation& allocation);

        /**
         * @brief Deallocate a previously allocated image.
         *
         * @param allocation Image allocation to deallocate.
         */
        void DeallocateImage(const ImageAllocation& allocation);

        /**
         * @brief Map memory for CPU access.
         *
         * @param allocation Allocation to map.
         * @return Expected<void*, VHResult> Pointer to mapped memory on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<void*, VHResult> MapMemory(const VmaAllocation& allocation);

        /**
         * @brief Unmap previously mapped memory.
         *
         * @param allocation Allocation to unmap.
         */
        void UnmapMemory(const VmaAllocation& allocation);

    private:
        VmaAllocator m_Allocator;

        /**
         * @brief Private constructor - use New() to create instances.
         *
         * @param allocator VMA allocator handle.
         */
        explicit VulkanMemoryAllocator(VmaAllocator allocator);
    };
}