#pragma once

#include "Core/Vector.h"

#include "VulkanMemoryAllocator.h"

#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

namespace VulkanHelper
{
    /**
     * @brief Internal class for deferred deletion of Vulkan objects.
     * 
     * This class manages a queue that delays the destruction of Vulkan objects
     * until a specified number of frames have passed. This is necessary in Vulkan
     * because objects may still be in use by the GPU even after the CPU has
     * finished with them.
     */
    class DeleteQueue
    {
    public:
        /**
         * @brief Constructs a DeleteQueue with specified delay.
         * 
         * @param device The Vulkan device to use for destruction calls
         * @param framesDelay Number of Update() calls to wait before destroying objects
         */
        explicit DeleteQueue(VkDevice device, VulkanMemoryAllocator* memoryAllocator, uint32_t framesDelay = 3);

        ~DeleteQueue();

        // Non-copyable, non-movable for simplicity since it's internal
        DeleteQueue(const DeleteQueue&) = delete;
        DeleteQueue& operator=(const DeleteQueue&) = delete;
        DeleteQueue(DeleteQueue&&) noexcept;
        DeleteQueue& operator=(DeleteQueue&&) noexcept;

        /**
         * @brief Advances the queue by one frame and destroys ready objects.
         * 
         * This should be called once per frame to process the deletion queue.
         */
        void Update();

        /**
         * @brief Queues a buffer for deferred deletion.
         */
        void QueueForDeletion(VkBuffer buffer);

        /**
         * @brief Queues an image for deferred deletion.
         */
        void QueueForDeletion(VkImage image);

        /**
         * @brief Queues an image view for deferred deletion.
         */
        void QueueForDeletion(VkImageView imageView);

        /**
         * @brief Queues a sampler for deferred deletion.
         */
        void QueueForDeletion(VkSampler sampler);

        /**
         * @brief Queues a pipeline for deferred deletion.
         */
        void QueueForDeletion(VkPipeline pipeline);

        /**
         * @brief Queues a pipeline layout for deferred deletion.
         */
        void QueueForDeletion(VkPipelineLayout pipelineLayout);

        /**
         * @brief Queues a descriptor set layout for deferred deletion.
         */
        void QueueForDeletion(VkDescriptorSetLayout descriptorSetLayout);

        /**
         * @brief Queues a descriptor pool for deferred deletion.
         */
        void QueueForDeletion(VkDescriptorPool descriptorPool);

        /**
         * @brief Queues a command pool for deferred deletion.
         */
        void QueueForDeletion(VkCommandPool commandPool);

        /**
         * @brief Queues a fence for deferred deletion.
         */
        void QueueForDeletion(VkFence fence);

        /**
         * @brief Queues a semaphore for deferred deletion.
         */
        void QueueForDeletion(VkSemaphore semaphore);

        /**
         * @brief Queues a swapchain for deferred deletion.
         */
        void QueueForDeletion(VkSwapchainKHR swapchain);

        /**
         * @brief Queues a shader module for deferred deletion.
         */
        void QueueForDeletion(VkShaderModule shaderModule);

        /**
         * @brief Queues a VulkanMemoryAllocator::BufferAllocation for deferred deletion.
         */
        void QueueForDeletion(VulkanMemoryAllocator::BufferAllocation allocation);

        /**
         * @brief Queues a VulkanMemoryAllocator::ImageAllocation for deferred deletion.
         */
        void QueueForDeletion(VulkanMemoryAllocator::ImageAllocation allocation);

        /**
         * @brief Flushes all pending deletions immediately.
         * 
         * This should only be used during shutdown or when you know
         * all GPU operations have completed.
         */
        void Flush();

    private:
        enum class ObjectType
        {
            BUFFER,
            IMAGE,
            IMAGE_VIEW,
            SAMPLER,
            PIPELINE,
            PIPELINE_LAYOUT,
            DESCRIPTOR_SET_LAYOUT,
            DESCRIPTOR_POOL,
            COMMAND_POOL,
            FENCE,
            SEMAPHORE,
            SWAPCHAIN,
            SHADER_MODULE,
            BUFFER_ALLOCATION,
            IMAGE_ALLOCATION
        };

        struct DeletionItem
        {
            ObjectType Type;
            uint64_t Handle; // Store as uint64_t to handle all Vulkan handle types
            uint32_t FramesToWait;
            VmaAllocation Allocation; // For VMA allocations, nullptr for regular Vulkan objects

            DeletionItem(ObjectType type, uint64_t handle, uint32_t frames, VmaAllocation allocation = nullptr)
                : Type(type), Handle(handle), FramesToWait(frames), Allocation(allocation) {}
        };

        void DestroyObject(const DeletionItem& item);

        VkDevice m_Device;
        uint32_t m_FramesDelay;
        VulkanHelper::Vector<DeletionItem> m_DeletionQueue;
        VulkanMemoryAllocator* m_MemoryAllocator;
    };
}
