#pragma once

#include "Core/Error.h"
#include "Core/SharedPtr.h"
#include "Core/Expected.h"

#include "Vulkan/Device.h"

namespace VulkanHelper
{
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
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

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

        Fence();

        Fence(const Fence& other);
        Fence& operator=(const Fence& other);

        Fence(Fence&& other) noexcept;
        Fence& operator=(Fence&& other) noexcept;

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
        VulkanHelper::SharedPtr<Impl> m_Impl;

        Fence(const SharedPtr<Impl>& impl);
    };
}