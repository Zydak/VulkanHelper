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
     * @brief RAII wrapper for Vulkan command pool objects
     * 
     * Manages allocation and lifecycle of command buffers.
     */
    class CommandPool
    {
    public:
        /**
         * @enum Flags
         * @brief Specifies command pool behavior
         */
        enum class Flags
        {
            TRANSIENT_BIT = 0x00000001,             ///< Pool allocates short-lived command buffers
            RESET_COMMAND_BUFFER_BIT = 0x00000002,   ///< Command buffers can be individually reset
            PROTECTED_BIT = 0x00000004,             ///< Command buffers are protected
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a command pool
         */
        struct Config
        {
            /**
             * @brief Device to create the command pool on
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief Flags controlling command pool behavior
             * @note Affects performance and functionality of allocated command buffers
             */
            VulkanHelper::CommandPool::Flags Flags = VulkanHelper::CommandPool::Flags::RESET_COMMAND_BUFFER_BIT;

            /**
             * @brief Queue family index for command buffer allocation
             * @note Must be a valid queue family index from the physical device
             */
            uint32_t QueueFamilyIndex = UINT32_MAX;
        };

        /**
         * @brief Delete copy constructor
         */
        CommandPool(const CommandPool& other) = delete;

        /**
         * @brief Delete copy assignment operator
         */
        CommandPool& operator=(const CommandPool& other) = delete;

        /**
         * @brief Move constructor
         */
        CommandPool(CommandPool&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        CommandPool& operator=(CommandPool&& other) noexcept;

        /**
         * @brief Destructor
         */
        ~CommandPool();

        /**
         * @brief Creates a new command pool
         * @param config Configuration parameters for the command pool
         * @return Expected containing the created command pool or an error code
         */
        [[nodiscard]] static VulkanHelper::Expected<CommandPool, VHResult> New(const Config& config);

        /**
         * @brief Allocates a command buffer from the pool
         * @param config Configuration parameters for the command buffer
         * @return Expected containing the allocated command buffer or an error code
         */
        [[nodiscard]] VulkanHelper::Expected<CommandBuffer, VHResult> AllocateCommandBuffer(const CommandBuffer::Config& config) const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl; 

        CommandPool(VulkanHelper::UniquePtr<Impl>&& impl);
    };

    DEFINE_BITWISE_OPERATORS(CommandPool::Flags)
}