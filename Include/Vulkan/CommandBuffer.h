#pragma once

#include "Core/SharedPtr.h"
#include "Core/Error.h"
#include "Core/Macros.h"
#include "Core/Enums.h"
#include "Core/Vector.h"

#include "Vulkan/Device.h"
#include "Vulkan/Fence.h"
#include "Vulkan/Semaphore.h"

namespace VulkanHelper
{
    /**
     * @class CommandBuffer
     * @brief RAII wrapper for Vulkan command buffer objects
     * 
     * Records and executes graphics and compute commands.
     */
    class CommandBuffer
    {
    public:
        /**
         * @enum Level
         * @brief Specifies the command buffer type
         */
        enum class Level
        {
            PRIMARY = 0,    ///< Can be submitted directly to a queue
            SECONDARY = 1   ///< Can only be executed from primary command buffers
        };

        /**
         * @enum Usage
         * @brief Specifies how the command buffer will be used
         */
        enum class Usage
        {
            NONE = 0,                           ///< No special usage
            ONE_TIME_SUBMIT_BIT = 0x00000001,   ///< Command buffer will be rewritten after execution
            RENDER_PASS_CONTINUE_BIT = 0x00000002, ///< Secondary buffer that continues a render pass
            SIMULTANEOUS_USE_BIT = 0x00000004,  ///< Can be submitted while still pending execution
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a command buffer
         */
        struct Config
        {
            /**
             * @brief Level of the command buffer to create
             * @note Determines how the command buffer can be used
             */
            VulkanHelper::CommandBuffer::Level Level = VulkanHelper::CommandBuffer::Level::PRIMARY;
        };

        CommandBuffer();

        CommandBuffer(const CommandBuffer& other);
        CommandBuffer& operator=(const CommandBuffer& other);

        CommandBuffer(CommandBuffer&& other) noexcept;
        CommandBuffer& operator=(CommandBuffer&& other) noexcept;

        ~CommandBuffer();

        /**
        * @brief Begins recording commands to the command buffer.
        *
        * @param usageFlags Flags indicating the intended usage of the command buffer.
        *
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult BeginRecording(Usage usageFlags);

        /**
        * @brief Ends recording commands to the command buffer.
        *
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult EndRecording();

        /**
        * @brief Submits the command buffer to the queue and blocks until execution of recorded commands is finished.
        *
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult SubmitAndWait();

        /**
        * @brief Submits the command buffer to the queue with given synchronization primitives.
        *
        * @param waitStage Pipeline stage to wait on before execution.
        * @param waitSemaphores Semaphores to wait on before execution.
        * @param signalSemaphores Semaphores to signal after execution.
        * @param fence Fence to signal after execution (can be nullptr).
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult Submit(PipelineStages waitStage, Vector<Semaphore> waitSemaphores, Vector<Semaphore> signalSemaphores, Fence* fence);

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        CommandBuffer(const SharedPtr<Impl>& impl);

        friend class CommandPool;
    };

    DEFINE_BITWISE_OPERATORS(CommandBuffer::Usage)
}