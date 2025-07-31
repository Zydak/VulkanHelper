#include "CommandBufferImpl.h"
#include "CommandPoolImpl.h"
#include "Core/Error.h"
#include "Vulkan/CommandBuffer.h"

#include <vulkan/vulkan.h>

#include "SemaphoreImpl.h"
#include "FenceImpl.h"

namespace VulkanHelper
{
    CommandBuffer::Impl::~Impl()
    {
        if (m_CommandBuffer != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying CommandBuffer Implementation");
            VH_ASSERT(m_CommandPool->FreeCommandBuffer(this) == VHResult::OK, "Failed to deallocate command buffer!");
        }
    }

    CommandBuffer::Impl::Impl(Impl&& other) noexcept
        : m_CommandPool(other.m_CommandPool), m_CommandBuffer(other.m_CommandBuffer)
    {
        other.m_CommandPool = nullptr;
        other.m_CommandBuffer = nullptr;
    }

    CommandBuffer::Impl& CommandBuffer::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl();

        m_CommandPool = other.m_CommandPool;
        other.m_CommandPool = nullptr;

        m_CommandBuffer = other.m_CommandBuffer;
        other.m_CommandBuffer = nullptr;

        return *this;
    }

    VHResult CommandBuffer::Impl::Begin(Usage usageFlags)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = (VkCommandBufferUsageFlags)usageFlags;

        return (VHResult)vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    }

    VHResult CommandBuffer::Impl::End()
    {
        return (VHResult)vkEndCommandBuffer(m_CommandBuffer);
    }

    VHResult CommandBuffer::Impl::SubmitAndWait()
    {
        VHResult res = Submit(PipelineStages::NONE, nullptr, nullptr, nullptr);
        if (res != VHResult::OK)
            return res;

        VkQueue queue = m_CommandPool->m_Queue;
        res = (VHResult)vkQueueWaitIdle(queue);
        if (res != VHResult::OK)
            return res;

        return VHResult::OK;
    }

    VHResult CommandBuffer::Impl::Submit(PipelineStages waitStage, Semaphore* waitSemaphore, Semaphore* signalSemaphore, Fence* fence)
    {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;

        VkPipelineStageFlags waitStageVk = VK_PIPELINE_STAGE_NONE;
        if (waitSemaphore != nullptr)
        {
            submitInfo.waitSemaphoreCount = 1;
            VkSemaphore waitSemaphoreVk = waitSemaphore->m_Impl->GetSemaphore();
            submitInfo.pWaitSemaphores = &waitSemaphoreVk;

            waitStageVk = (VkPipelineStageFlags)waitStage;
            submitInfo.pWaitDstStageMask = &waitStageVk;
        }

        if (signalSemaphore != nullptr)
        {
            submitInfo.signalSemaphoreCount = 1;
            VkSemaphore signalSemaphoreVk = signalSemaphore->m_Impl->GetSemaphore();
            submitInfo.pSignalSemaphores = &signalSemaphoreVk;
        }

        VkFence fenceVk = VK_NULL_HANDLE;
        {
            fenceVk = fence->m_Impl->GetFenceHandle();
        }

        VkQueue queue = m_CommandPool->m_Queue;
        VkResult res = vkQueueSubmit(queue, 1, &submitInfo, fenceVk);
        if (res != VK_SUCCESS)
            return VHResult(res);

        return VHResult::OK;
    }

    //
    //  Forward Functions
    //

    CommandBuffer::CommandBuffer(CommandBuffer&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {

    }

    CommandBuffer& CommandBuffer::operator=(CommandBuffer&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~CommandBuffer(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    CommandBuffer::~CommandBuffer()
    {

    }

    CommandBuffer::CommandBuffer(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    VHResult CommandBuffer::Begin(Usage usageFlags)
    {
        return m_Impl->Begin(usageFlags);
    }

    VHResult CommandBuffer::End()
    {
        return m_Impl->End();
    }
    
    VHResult CommandBuffer::SubmitAndWait()
    {
        return m_Impl->SubmitAndWait();
    }

    VHResult CommandBuffer::Submit(PipelineStages waitStage, Semaphore* waitSemaphore, Semaphore* signalSemaphore, Fence* fence)
    {
        return m_Impl->Submit(waitStage, waitSemaphore, signalSemaphore, fence);
    }
}