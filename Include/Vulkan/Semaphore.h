#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"

namespace VulkanHelper
{
    class Device;

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
             * 
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;
        };

        /**
         * @brief Creates a new binary semaphore for GPU queue synchronization.
         * 
         * @param config Semaphore creation configuration
         * @return Expected<Semaphore, VHResult> The created semaphore or error code
         * @note Device must be valid
         */
        [[nodiscard]] static Expected<Semaphore, VHResult> New(const Config& config);

        Semaphore(const Semaphore& other) = delete;
        Semaphore& operator=(const Semaphore& other) = delete;

        Semaphore(Semaphore&& other) noexcept;
        Semaphore& operator=(Semaphore&& other) noexcept;

        ~Semaphore();

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Semaphore(UniquePtr<Impl>&& impl);
    };
}