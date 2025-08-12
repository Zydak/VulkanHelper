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
        [[nodiscard]] inline static CommandBuffer CreatePublicInterface(Impl&& impl) { return CommandBuffer(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<CommandBuffer> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new CommandBuffer(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        [[nodiscard]] VHResult BeginRecording(Usage usageFlags);
        [[nodiscard]] VHResult EndRecording();

        [[nodiscard]] VHResult SubmitAndWait();
        [[nodiscard]] VHResult Submit(
            PipelineStages waitStage,
            const Vector<Semaphore::Impl*>& waitSemaphores,
            const Vector<Semaphore::Impl*>& signalSemaphores,
            Fence::Impl* fence
        );

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