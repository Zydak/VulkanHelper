#pragma once

#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Expected.h"

namespace VulkanHelper
{
    class Device;

    /**
     * @class Fence
     * @brief RAII wrapper for Vulkan fence objects
     * 
     * Provides CPU-GPU synchronization.
     */
    class Fence
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a fence
         */
        struct Config
        {
            /**
             * @brief Device to create the fence on
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief Initial state of the fence
             * @note When true, fence starts signaled and doesn't need initial wait
             */
            bool StartSignaled = true;
        };

        /**
         * @brief Creates a new fence
         * @param config Configuration parameters for the fence
         * @return Expected containing the created fence or an error code
         */
        [[nodiscard]] static Expected<Fence, VHResult> New(const Config& config);

        /**
         * @brief Delete copy constructor
         */
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
        
        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Fence(UniquePtr<Impl>&& impl);
    };
}