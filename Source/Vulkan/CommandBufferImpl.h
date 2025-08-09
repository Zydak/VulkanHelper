#pragma once
#include "Vulkan/CommandPool.h"
#include "Vulkan/Semaphore.h"
#include "Vulkan/Fence.h"

#include <vulkan/vulkan.h>

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
        [[nodiscard]] inline static CommandBuffer CreatePublicInterface(UniquePtr<Impl>&& impl) { return CommandBuffer(VulkanHelper::Move(impl)); }

        [[nodiscard]] VHResult BeginRecording(Usage usageFlags);
        [[nodiscard]] VHResult EndRecording();

        [[nodiscard]] VHResult SubmitAndWait();
        [[nodiscard]] VHResult Submit(PipelineStages waitStage, Semaphore** waitSemaphore, uint32_t waitSemaphoreCount, Semaphore** signalSemaphore, uint32_t signalSemaphoreCount, Fence* fence);

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