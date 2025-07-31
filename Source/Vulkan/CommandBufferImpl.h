#pragma once
#include "Vulkan/CommandPool.h"
#include "Vulkan/Semaphore.h"
#include "Vulkan/Fence.h"

typedef struct VkCommandBuffer_T* VkCommandBuffer;

namespace VulkanHelper
{
    class CommandBuffer::Impl
    {
    public:
        ~Impl();

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const CommandBuffer* publicInterface) { return publicInterface->m_Impl.Get(); }

        [[nodiscard]] VHResult Begin(Usage usageFlags);
        [[nodiscard]] VHResult End();

        [[nodiscard]] VHResult SubmitAndWait();
        [[nodiscard]] VHResult Submit(PipelineStages waitStage, Semaphore* waitSemaphore, Semaphore* signalSemaphore, Fence* fence);

        [[nodiscard]] inline VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
    private:
        Impl(VulkanHelper::CommandPool::Impl* pool, VkCommandBuffer commandBuffer)
            : m_CommandPool(pool), m_CommandBuffer(commandBuffer)
        {

        }

        VulkanHelper::CommandPool::Impl* m_CommandPool;
        VkCommandBuffer m_CommandBuffer;

        friend class CommandPool; // Allow CommandPool to create CommandBuffer::Impl instances
    };
}