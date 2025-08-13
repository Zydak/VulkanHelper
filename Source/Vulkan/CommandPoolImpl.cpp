#include "Vulkan/CommandPool.h"
#include "CommandPoolImpl.h"
#include "CommandBufferImpl.h"

#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"
#include "Core/Error.h"

namespace VulkanHelper
{
    Expected<SharedPtr<CommandPool::Impl>, VHResult> CommandPool::Impl::New(
        const SharedPtr<Device::Impl>& device,
        Flags flags,
        uint32_t queueFamilyIndex
    )
    {
        VH_LOG_INFO("Creating Vulkan CommandPool Implementation");

        if (device == nullptr)
        {
            VH_LOG_ERROR("Invalid CommandPool configuration: Device is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VkCommandPoolCreateFlags(flags);

        VkCommandPool commandPool;
        VkResult res = vkCreateCommandPool(device->GetDevice(), &poolInfo, nullptr, &commandPool);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create command pool implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        VkQueue queue;
        vkGetDeviceQueue(device->GetDevice(), queueFamilyIndex, 0, &queue);
        if (queue == VK_NULL_HANDLE)
        {
            VH_LOG_ERROR("Failed to get queue for command pool implementation, is the queue index correct? Queue index: {}", queueFamilyIndex);
            vkDestroyCommandPool(device->GetDevice(), commandPool, nullptr);
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        return SharedPtr<Impl>(new Impl(device, commandPool, queue, flags, queueFamilyIndex));
    }

    CommandPool::Impl::~Impl()
    {
        if (m_CommandPool != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan Command Pool Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_CommandPool);
            m_CommandPool = VK_NULL_HANDLE;
            m_Device = nullptr;
        }
    }

    CommandPool::Impl::Impl(CommandPool::Impl&& other) noexcept
        : m_Device(other.m_Device), m_CommandPool(other.m_CommandPool), m_Queue(other.m_Queue), m_Flags(other.m_Flags), m_QueueFamilyIndex(other.m_QueueFamilyIndex)
    {
        other.m_CommandPool = VK_NULL_HANDLE;
        other.m_Device = nullptr;
    }

    CommandPool::Impl& CommandPool::Impl::operator=(CommandPool::Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_Device = other.m_Device;
        m_CommandPool = other.m_CommandPool;
        m_Queue = other.m_Queue;
        m_Flags = other.m_Flags;
        m_QueueFamilyIndex = other.m_QueueFamilyIndex;

        other.m_CommandPool = VK_NULL_HANDLE;
        other.m_Device = nullptr;

        return *this;
    }

    VulkanHelper::Expected<CommandBuffer, VHResult> CommandPool::Impl::AllocateCommandBuffer(const SharedPtr<Impl>& impl, CommandBuffer::Level level)
    {
        VH_LOG_INFO("Allocating Command Buffer");
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = impl->m_CommandPool;
        allocInfo.level = (VkCommandBufferLevel)level;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        VkResult res = vkAllocateCommandBuffers(impl->m_Device->GetDevice(), &allocInfo, &commandBuffer);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to allocate command buffer");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return CommandBuffer{ SharedPtr<CommandBuffer::Impl>( new CommandBuffer::Impl(impl, commandBuffer)) };
    }

    VHResult CommandPool::Impl::FreeCommandBuffer(CommandBuffer::Impl* commandBuffer) const
    {
        if (commandBuffer->m_CommandPool != this)
        {
            VH_LOG_ERROR("Command buffer does not belong to this command pool");
            return VHResult::WRONG_ARGUMENTS;
        }

        VH_LOG_INFO("Freeing Command Buffer");
        vkFreeCommandBuffers(m_Device->GetDevice(), m_CommandPool, 1, &commandBuffer->m_CommandBuffer);
        commandBuffer->m_CommandBuffer = VK_NULL_HANDLE;
        commandBuffer->m_CommandPool = nullptr;
        return VHResult::OK;
    }

    //
    //  Forward Functions
    //

    VulkanHelper::Expected<CommandPool, VHResult> CommandPool::New(const Config& config)
    {
        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.Flags,
            config.QueueFamilyIndex
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    VulkanHelper::Expected<CommandBuffer, VHResult> CommandPool::AllocateCommandBuffer(const CommandBuffer::Config& config) const
    {
        return m_Impl->AllocateCommandBuffer(m_Impl, config.Level);
    }

    CommandPool::CommandPool()
        : m_Impl(nullptr)
    {
    }

    CommandPool::CommandPool(const CommandPool& other)
        : m_Impl(other.m_Impl)
    {
    }

    CommandPool& CommandPool::operator=(const CommandPool& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;
        return *this;
    }

    CommandPool::CommandPool(CommandPool&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {

    }

    CommandPool& CommandPool::operator=(CommandPool&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~CommandPool(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    CommandPool::~CommandPool()
    {

    }

    CommandPool::CommandPool(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }
}