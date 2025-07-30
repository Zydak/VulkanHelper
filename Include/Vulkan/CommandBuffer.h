#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Macros.h"

namespace VulkanHelper
{
    class CommandPool;
    class Semaphore;
    class Fence;

    /**
    * @class CommandBuffer
    * @brief Vulkan command buffer wrapper.
    *
    * Provides a RAII wrapper for Vulkan command buffers, supporting primary and secondary levels.
    */
    class CommandBuffer
    {
    public:
        enum class Level
        {
            PRIMARY = 0,
            SECONDARY = 1
        };

        enum class UsageFlags
        {
            NONE = 0,
            ONE_TIME_SUBMIT_BIT = 0x00000001,
            RENDER_PASS_CONTINUE_BIT = 0x00000002,
            SIMULTANEOUS_USE_BIT = 0x00000004,
        };

        enum class WaitStages
        {
            NONE = 0,
            TOP_OF_PIPE_BIT = 0x00000001,
            DRAW_INDIRECT_BIT = 0x00000002,
            VERTEX_INPUT_BIT = 0x00000004,
            VERTEX_SHADER_BIT = 0x00000008,
            TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
            TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
            GEOMETRY_SHADER_BIT = 0x00000040,
            FRAGMENT_SHADER_BIT = 0x00000080,
            EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
            LATE_FRAGMENT_TESTS_BIT = 0x00000200,
            COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
            COMPUTE_SHADER_BIT = 0x00000800,
            TRANSFER_BIT = 0x00001000,
            BOTTOM_OF_PIPE_BIT = 0x00002000,
            HOST_BIT = 0x00004000,
            ALL_GRAPHICS_BIT = 0x00008000,
            ALL_COMMANDS_BIT = 0x00010000,
            TRANSFORM_FEEDBACK_BIT_EXT = 0x01000000,
            CONDITIONAL_RENDERING_BIT_EXT = 0x00040000,
            ACCELERATION_STRUCTURE_BUILD_BIT_KHR = 0x02000000,
            RAY_TRACING_SHADER_BIT_KHR = 0x00200000,
            FRAGMENT_DENSITY_PROCESS_BIT_EXT = 0x00800000,
            FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR = 0x00400000,
            COMMAND_PREPROCESS_BIT_NV = 0x00020000,
            TASK_SHADER_BIT_EXT = 0x00080000,
            MESH_SHADER_BIT_EXT = 0x00100000,
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
        *
        * @param other The CommandBuffer to move from.
        */
        CommandBuffer(CommandBuffer&& other) noexcept;
        
        /**
        * @brief Move assignment operator. Takes ownership of another CommandBuffer's resources.
        *
        * @param other The CommandBuffer to move from.
        *
        * @return Reference to this CommandBuffer.
        */
        CommandBuffer& operator=(CommandBuffer&& other) noexcept;

        /**
        * @brief Destructor. Destroys the CommandBuffer and releases resources.
        */
        ~CommandBuffer();

        /**
        * @brief Begins recording commands to the command buffer.
        *
        * @param usageFlags Flags indicating the intended usage of the command buffer.
        *
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult Begin(UsageFlags usageFlags);

        /**
        * @brief Ends recording commands to the command buffer.
        *
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult End();

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
        * @param waitSemaphore Semaphore to wait on before execution (can be nullptr).
        * @param signalSemaphore Semaphore to signal after execution (can be nullptr).
        * @param fence Fence to signal after execution (can be nullptr).
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult Submit(WaitStages waitStage, Semaphore* waitSemaphore, Semaphore* signalSemaphore, Fence* fence);

    private:

        class Impl;
        UniquePtr<Impl> m_Impl;

        /**
        * @brief Private constructor used by the factory function.
        * @param impl Implementation pointer.
        */
        CommandBuffer(UniquePtr<Impl>&& impl);

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

        #undef COMMAND_BUFFER_CLASS
        DECLARE_FRIENDS();
        #define COMMAND_BUFFER_CLASS CommandBuffer
    };
}