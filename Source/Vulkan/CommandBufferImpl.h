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

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const CommandBuffer& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static CommandBuffer CreatePublicInterface(const SharedPtr<Impl>& impl) { return CommandBuffer(impl); }

        [[nodiscard]] VHResult BeginRecording(Usage usageFlags);
        [[nodiscard]] VHResult EndRecording();

        [[nodiscard]] VHResult SubmitAndWait();
        [[nodiscard]] VHResult Submit(
            PipelineStages waitStage,
            const Vector<SharedPtr<Semaphore::Impl>>& waitSemaphores,
            const Vector<SharedPtr<Semaphore::Impl>>& signalSemaphores,
            SharedPtr<Fence::Impl> fence
        );

        [[nodiscard]] inline VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
    private:
        Impl(const SharedPtr<CommandPool::Impl>& pool, VkCommandBuffer commandBuffer);

        SharedPtr<CommandPool::Impl> m_CommandPool;
        VkCommandBuffer m_CommandBuffer;

        friend class CommandPool; // Allow CommandPool to create CommandBuffer::Impl instances
    };
}