#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"

namespace VulkanHelper
{
    class CommandPool;

    class CommandBuffer
    {
    public:
        enum class Level
        {
            PRIMARY = 0,
            SECONDARY = 1
        };

        struct Config
        {
            VulkanHelper::CommandBuffer::Level Level = VulkanHelper::CommandBuffer::Level::PRIMARY;
        };

        CommandBuffer(const CommandBuffer& other) = delete;
        CommandBuffer& operator=(const CommandBuffer& other) = delete;

        CommandBuffer(CommandBuffer&& other) noexcept;
        CommandBuffer& operator=(CommandBuffer&& other) noexcept;

        ~CommandBuffer();

    private:
        class Impl;
        UniquePtr<Impl> m_Impl;

        CommandBuffer(UniquePtr<Impl>&& impl)
            : m_Impl(std::move(impl))
        {}

        [[nodiscard]] VulkanHelper::Expected<CommandBuffer, VHResult> New(const Config& config);

        friend class CommandPool; // Allow CommandPool to create CommandBuffer instances
    };
}