#pragma once

#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Move.h"
#include "Core/Expected.h"

namespace VulkanHelper
{
    class Device;

    /**
    * @class Fence
    * @brief Vulkan synchronization primitive wrapper.
    *
    * Provides a RAII wrapper for Vulkan fences, allowing for easy waiting and resetting.
    */
    class Fence
    {
    public:
        /**
        * @struct Config
        * @brief Configuration parameters for creating a Fence instance.
        */
        struct Config
        {
            /**
            * @brief Pointer to the logical Vulkan device to use for fence creation.
            *
            * @note Cannot be nullptr.
            */
            VulkanHelper::Device* Device = nullptr;

            /**
            * @brief Whether the fence should start in the signaled state.
            */
            bool StartSignaled = true;
        };

        /**
        * @brief Creates a new Fence instance with the specified configuration.
        *
        * This static factory function attempts to create a Fence according to the provided configuration.
        * If successful, it returns a Fence object; otherwise, it returns a VHResult describing the failure.
        *
        * @param config The configuration struct specifying fence creation options.
        * @return VulkanHelper::Expected<Fence, VHResult> An expected containing the created Fence on success, or a VHResult on failure.
        */
        [[nodiscard]] static Expected<Fence, VHResult> New(const Config& config);

        Fence(const Fence& other) = delete;
        Fence& operator=(const Fence& other) = delete;

        /**
        * @brief Move constructor. Takes ownership of another Fence's resources.
        * @param other The Fence to move from.
        */
        Fence(Fence&& other) noexcept;

        /**
        * @brief Move assignment operator. Takes ownership of another Fence's resources.
        * @param other The Fence to move from.
        * @return Reference to this Fence.
        */
        Fence& operator=(Fence&& other) noexcept;

        /**
        * @brief Destructor. Destroys the Fence and releases resources.
        */
        ~Fence();

        /**
        * @brief Waits for the fence to be signaled.
        *
        * Blocks until the fence is signaled.
        */
        void Wait();
        
        /**
        * @brief Resets the fence to the unsignaled state.
        */
        void Reset();
    private:
        class Impl;
        UniquePtr<Impl> m_Impl;

        /**
        * @brief Private constructor used by the factory function.
        * @param impl Implementation pointer.
        */
        Fence(UniquePtr<Impl>&& impl)
            : m_Impl(VulkanHelper::Move(impl))
        {}

        friend class Swapchain;
    };
}