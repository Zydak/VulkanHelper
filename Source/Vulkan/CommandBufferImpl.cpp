#include "CommandBufferImpl.h"
#include "CommandPoolImpl.h"
#include "Vulkan/CommandBuffer.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    CommandBuffer::Impl::~Impl()
    {
        if (m_CommandBuffer != VK_NULL_HANDLE)
            VH_ASSERT(m_CommandPool->FreeCommandBuffer(this) == VHResult::OK, "Failed to deallocate command buffer!");
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
        VH_LOG_INFO("Destroying CommandBuffer");
    }
}