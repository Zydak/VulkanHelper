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

    VHResult CommandBuffer::Impl::BeginRecording(Usage usageFlags)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = (VkCommandBufferUsageFlags)usageFlags;

        return (VHResult)vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    }

    VHResult CommandBuffer::Impl::EndRecording()
    {
        return (VHResult)vkEndCommandBuffer(m_CommandBuffer);
    }

    VHResult CommandBuffer::Impl::SubmitAndWait()
    {
        VHResult res = Submit(PipelineStages::NONE, {}, {}, nullptr);
        if (res != VHResult::OK)
            return res;

        VkQueue queue = m_CommandPool->m_Queue;
        res = (VHResult)vkQueueWaitIdle(queue);
        if (res != VHResult::OK)
            return res;

        return VHResult::OK;
    }

    VHResult CommandBuffer::Impl::Submit(
        PipelineStages waitStage,
        const Vector<Semaphore::Impl*>& waitSemaphores,
        const Vector<Semaphore::Impl*>& signalSemaphores,
        Fence::Impl* fence
    )
    {
        if (m_CommandPool->m_Device == nullptr)
        {
            VH_LOG_ERROR("Command pool device is null, cannot submit command buffer");
            return VHResult::WRONG_ARGUMENTS;
        }
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_CommandBuffer;

        VkPipelineStageFlags waitStageVk = VK_PIPELINE_STAGE_NONE;
        Vector<VkSemaphore> waitSemaphoresVk(waitSemaphores.Size(), VK_NULL_HANDLE);
        if (!waitSemaphores.Empty())
        {
            for (size_t i = 0; i < waitSemaphores.Size(); i++)
            {
                waitSemaphoresVk[i] = waitSemaphores[i]->GetSemaphore();
            }
            submitInfo.pWaitSemaphores = waitSemaphoresVk.Data();
            
            waitStageVk = (VkPipelineStageFlags)waitStage;
            submitInfo.waitSemaphoreCount = waitSemaphores.Size();
            submitInfo.pWaitDstStageMask = &waitStageVk;
        }

        Vector<VkSemaphore> submitSemaphores(signalSemaphores.Size(), VK_NULL_HANDLE);
        if (!signalSemaphores.Empty())
        {
            for (uint32_t i = 0; i < signalSemaphores.Size(); i++)
            {
                submitSemaphores[i] = signalSemaphores[i]->GetSemaphore();
            }
            submitInfo.pSignalSemaphores = submitSemaphores.Data();
            submitInfo.signalSemaphoreCount = signalSemaphores.Size();
        }

        VkFence fenceVk = VK_NULL_HANDLE;
        if (fence != nullptr)
            fenceVk = fence->GetFenceHandle();

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

    VHResult CommandBuffer::BeginRecording(Usage usageFlags)
    {
        return m_Impl->BeginRecording(usageFlags);
    }

    VHResult CommandBuffer::EndRecording()
    {
        return m_Impl->EndRecording();
    }
    
    VHResult CommandBuffer::SubmitAndWait()
    {
        return m_Impl->SubmitAndWait();
    }

    VHResult CommandBuffer::Submit(PipelineStages waitStage, Vector<Semaphore*> waitSemaphores, Vector<Semaphore*> signalSemaphores, Fence* fence)
    {
        Vector<Semaphore::Impl*> waitSemaphoresImpl(waitSemaphores.Size());
        for (size_t i = 0; i < waitSemaphores.Size(); i++)
        {
            waitSemaphoresImpl[i] = Semaphore::Impl::GetImplementation(waitSemaphores[i]);
        }
        Vector<Semaphore::Impl*> signalSemaphoresImpl(signalSemaphores.Size());
        for (size_t i = 0; i < signalSemaphores.Size(); i++)
        {
            signalSemaphoresImpl[i] = Semaphore::Impl::GetImplementation(signalSemaphores[i]);
        }

        Fence::Impl* fenceImpl = nullptr;
        if (fence != nullptr)
            fenceImpl = Fence::Impl::GetImplementation(fence);

        return m_Impl->Submit(waitStage, waitSemaphoresImpl, signalSemaphoresImpl, fenceImpl);
    }
}