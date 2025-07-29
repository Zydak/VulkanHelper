#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"

namespace VulkanHelper
{
    class CommandPool;

    /**
    * @class CommandBuffer
    * @brief Vulkan command buffer wrapper.
    *
    * Provides a RAII wrapper for Vulkan command buffers, supporting primary and secondary levels.
    */
    class CommandBuffer
    {
    public:
        /**
        * @enum Level
        * @brief Command buffer level (primary or secondary).
        */
        enum class Level
        {
            PRIMARY = 0,
            SECONDARY = 1
        };

        /**
        * @struct Config
        * @brief Configuration parameters for creating a CommandBuffer instance.
        */
        struct Config
        {
            /**
            * @brief Level of the command buffer (primary or secondary).
            */
            VulkanHelper::CommandBuffer::Level Level = VulkanHelper::CommandBuffer::Level::PRIMARY;
        };

        CommandBuffer(const CommandBuffer& other) = delete;
        CommandBuffer& operator=(const CommandBuffer& other) = delete;

        /**
        * @brief Move constructor. Takes ownership of another CommandBuffer's resources.
        * @param other The CommandBuffer to move from.
        */
        CommandBuffer(CommandBuffer&& other) noexcept;
        
        /**
        * @brief Move assignment operator. Takes ownership of another CommandBuffer's resources.
        * @param other The CommandBuffer to move from.
        * @return Reference to this CommandBuffer.
        */
        CommandBuffer& operator=(CommandBuffer&& other) noexcept;

        /**
        * @brief Destructor. Destroys the CommandBuffer and releases resources.
        */
        ~CommandBuffer();

    private:
        class Impl;
        UniquePtr<Impl> m_Impl;

        /**
        * @brief Private constructor used by the factory function.
        * @param impl Implementation pointer.
        */
        CommandBuffer(UniquePtr<Impl>&& impl)
            : m_Impl(VulkanHelper::Move(impl))
        {}

        /**
        * @brief Creates a new CommandBuffer instance with the specified configuration.
        *
        * This static factory function attempts to create a CommandBuffer according to the provided configuration.
        * If successful, it returns a CommandBuffer object; otherwise, it returns a VHResult describing the failure.
        *
        * @param config The configuration struct specifying command buffer creation options.
        * @return VulkanHelper::Expected<CommandBuffer, VHResult> An expected containing the created CommandBuffer on success, or a VHResult on failure.
        */
        [[nodiscard]] VulkanHelper::Expected<CommandBuffer, VHResult> New(const Config& config);

        friend class CommandPool; // Allow CommandPool to create CommandBuffer instances
    };
}