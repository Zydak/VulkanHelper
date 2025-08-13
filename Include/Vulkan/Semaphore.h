#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/SharedPtr.h"

#include "Vulkan/Device.h"

namespace VulkanHelper
{
    /**
     * @class Semaphore
     * @brief RAII wrapper for a Vulkan synchronization semaphore. Used for GPU queue synchronization.
     */
    class Semaphore
    {
    public:
        /**
         * @brief Configuration for semaphore creation
         */
        struct Config
        {
            /**
             * @brief The logical device that will own this semaphore
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};
        };

        /**
         * @brief Creates a new binary semaphore for GPU queue synchronization.
         * 
         * @param config Semaphore creation configuration
         * @return Expected<Semaphore, VHResult> The created semaphore or error code
         * @note Device must be valid
         */
        [[nodiscard]] static Expected<Semaphore, VHResult> New(const Config& config);

        Semaphore();

        Semaphore(const Semaphore& other);
        Semaphore& operator=(const Semaphore& other);

        Semaphore(Semaphore&& other) noexcept;
        Semaphore& operator=(Semaphore&& other) noexcept;

        ~Semaphore();

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        Semaphore(const SharedPtr<Impl>& impl);
    };
}