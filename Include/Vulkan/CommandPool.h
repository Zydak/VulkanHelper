#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Macros.h"

#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    class Device;
    
    /**
    * @class CommandPool
    * @brief Vulkan command pool wrapper.
    *
    * Provides a RAII wrapper for Vulkan command pools, allowing for allocation of command buffers.
    */
    class CommandPool
    {
    public:
        /**
        * @enum Flags
        * @brief Flags for command pool creation.
        */
        enum class Flags
        {
            TRANSIENT_BIT = 0x00000001,
            RESET_COMMAND_BUFFER_BIT = 0x00000002,
            PROTECTED_BIT = 0x00000004,
        };

        /**
        * @struct Config
        * @brief Configuration parameters for creating a CommandPool instance.
        *
        * Specify the logical device, flags, and queue family index.
        */
        struct Config
        {
            /**
            * @brief Pointer to the logical Vulkan device to use for command pool creation.
            *
            * @note Cannot be nullptr.
            */
            VulkanHelper::Device* Device = nullptr;

            /**
            * @brief Flags controlling command pool behavior.
            */
            VulkanHelper::CommandPool::Flags Flags = VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT;

            /**
            * @brief Queue family index for the command pool.
            */
            uint32_t QueueFamilyIndex = UINT32_MAX;
        };

        CommandPool(const CommandPool& other) = delete;
        CommandPool& operator=(const CommandPool& other) = delete;

        /**
        * @brief Move constructor. Takes ownership of another CommandPool's resources.
        * @param other The CommandPool to move from.
        */
        CommandPool(CommandPool&& other) noexcept;

        /**
        * @brief Move assignment operator. Takes ownership of another CommandPool's resources.
        * @param other The CommandPool to move from.
        * @return Reference to this CommandPool.
        */
        CommandPool& operator=(CommandPool&& other) noexcept;

        /**
        * @brief Destructor. Destroys the CommandPool and releases resources.
        */
        ~CommandPool();

        /**
        * @brief Creates a new CommandPool instance with the specified configuration.
        *
        * This static factory function attempts to create a CommandPool according to the provided configuration.
        * If successful, it returns a CommandPool object; otherwise, it returns a VHResult describing the failure.
        *
        * @param config The configuration struct specifying command pool creation options.
        * @return VulkanHelper::Expected<CommandPool, VHResult> An expected containing the created CommandPool on success, or a VHResult on failure.
        */
        [[nodiscard]] static VulkanHelper::Expected<CommandPool, VHResult> New(const Config& config);

        /**
        * @brief Allocates a command buffer from the pool.
        *
        * @param config The configuration struct specifying command buffer allocation options.
        * @return VulkanHelper::Expected<CommandBuffer, VHResult> An expected containing the allocated CommandBuffer on success, or a VHResult on failure.
        */
        [[nodiscard]] VulkanHelper::Expected<CommandBuffer, VHResult> AllocateCommandBuffer(const CommandBuffer::Config& config) const;

    private:
        class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        /**
        * @brief Private constructor used by the factory function.
        * @param impl Implementation pointer.
        */
        CommandPool(VulkanHelper::UniquePtr<Impl>&& impl);

        #undef COMMAND_POOL_CLASS
        DECLARE_FRIENDS();
        #define COMMAND_POOL_CLASS CommandPool
    };

    DEFINE_BITWISE_OPERATORS(CommandPool::Flags)
}