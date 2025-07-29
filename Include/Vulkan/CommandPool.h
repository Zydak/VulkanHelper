#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Device.h"

#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    class CommandPool
    {
    public:
        enum class Flags
        {
            TRANSIENT_BIT = 0x00000001,
            RESET_COMMAND_BUFFER_BIT = 0x00000002,
            PROTECTED_BIT = 0x00000004,
        };

        struct Config
        {
            VulkanHelper::Device* Device = nullptr;
            VulkanHelper::CommandPool::Flags Flags = VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT;
            uint32_t QueueFamilyIndex = 0;
        };

        CommandPool(const CommandPool& other) = delete;
        CommandPool& operator=(const CommandPool& other) = delete;

        CommandPool(CommandPool&& other) noexcept;
        CommandPool& operator=(CommandPool&& other) noexcept;

        ~CommandPool();

        [[nodiscard]] static VulkanHelper::Expected<CommandPool, VHResult> New(const Config& config);

        [[nodiscard]] VulkanHelper::Expected<CommandBuffer, VHResult> AllocateCommandBuffer(const CommandBuffer::Config& config) const;

    private:
        class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        CommandPool(VulkanHelper::UniquePtr<Impl>&& impl)
            : m_Impl(std::move(impl))
        {}

        friend class CommandBuffer; // Allow CommandBuffer to access implementation directly
    };
}