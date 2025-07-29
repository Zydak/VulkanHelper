#pragma once
#include "Vulkan/CommandPool.h"

typedef struct VkQueue_T* VkQueue;
typedef struct VkCommandPool_T* VkCommandPool;

namespace VulkanHelper
{
    class CommandPool::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Config& config);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] VulkanHelper::Expected<CommandBuffer, VHResult> AllocateCommandBuffer(const CommandBuffer::Config& config);
        
    private:
        Impl(Device::Impl* device, VkCommandPool commandPool, VkQueue queue, Flags flags, uint32_t queueFamilyIndex)
        : m_Device(device), m_CommandPool(commandPool), m_Queue(queue), m_Flags(flags), m_QueueFamilyIndex(queueFamilyIndex)
        {}

        [[nodiscard]] VHResult FreeCommandBuffer(CommandBuffer::Impl* commandBuffer) const;

        Device::Impl* m_Device;
        VkCommandPool m_CommandPool;
        VkQueue m_Queue;
        Flags m_Flags;
        uint32_t m_QueueFamilyIndex;

        friend class CommandBuffer::Impl; // Allow access to FreeCommandBuffer
    };
}