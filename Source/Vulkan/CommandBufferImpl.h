#pragma once
#include "Vulkan/CommandPool.h"

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

        [[nodisacrd]] VHResult Begin();
        [[nodiscard]] VHResult End();
    private:
        Impl(VulkanHelper::CommandPool::Impl* pool, VkCommandBuffer commandBuffer)
            : m_CommandPool(pool), m_CommandBuffer(commandBuffer)
        {}

        VulkanHelper::CommandPool::Impl* m_CommandPool;
        VkCommandBuffer m_CommandBuffer;

        friend class CommandPool; // Allow CommandPool to create CommandBuffer::Impl instances
    };
}