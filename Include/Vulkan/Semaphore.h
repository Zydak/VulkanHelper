#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"

namespace VulkanHelper
{
    class Device;

    class Semaphore
    {
    public:
        /**
        * @struct Config
        * @brief Configuration parameters for creating a Semaphore instance.
        *
        * Specify the logical device to use for semaphore creation (can't be nullptr).
        */
        struct Config
        {
            VulkanHelper::Device* Device = nullptr;
        };

        /**
        * @brief Creates a new Semaphore instance with the specified configuration.
        *
        * @param config The configuration struct specifying semaphore creation options.
        *
        * @return Expected<Semaphore, VHResult> An expected containing the created Semaphore on success, or a VHResult on failure.
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