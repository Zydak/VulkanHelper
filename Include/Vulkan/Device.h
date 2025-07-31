#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Vulkan/PhysicalDevice.h"

namespace VulkanHelper
{
    class Window;
    
    /**
     * @class Device
     * @brief RAII wrapper for a Vulkan logical device.
     *
     * Manages the lifetime of a Vulkan logical device, including queues and command pools.
     */
    class Device
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a Device instance.
         *
         * Specify the physical device to use and, optionally, a window for presentation support.
         * Pass this struct to Device::New() to create a logical device and associated resources.
         */
        struct Config
        {
            /**
             * @brief The Vulkan physical device to use for logical device creation.
             *
             * This must be a valid and suitable physical device selected by the application.
             */
            VulkanHelper::PhysicalDevice PhysicalDevice;

            /**
             * @brief Vector containing pointers to windows
             *
             * If vector is not empty, it queries support for presenting queue that supports all listed windows.
             */
            VulkanHelper::Vector<VulkanHelper::Window*> Windows;
        };

        struct QueueFamilyIndices
        {
            uint32_t GraphicsFamily = UINT32_MAX;
            uint32_t ComputeFamily = UINT32_MAX;
            uint32_t PresentFamily = UINT32_MAX;
        };

        /**
         * @brief Creates a new logical Vulkan device and associated queues/command pools.
         *
         * This static factory function attempts to create a logical device from the given physical device and optional window.
         * It selects appropriate queue families for graphics, compute, and presentation, and creates command pools for each.
         *
         * @param config Configuration struct specifying the physical device and optional window for presentation support.
         * @return VulkanHelper::Expected<Device, VHError> An expected containing the created Device on success, or a VHError on failure.
         */
        [[nodiscard]] static VulkanHelper::Expected<Device, VHResult> New(const Config& config);
        ~Device();
        Device(const Device& other) = delete;
        Device& operator=(const Device& other) = delete;
        Device(Device&& other) noexcept;
        Device& operator=(Device&& other) noexcept;

        /**
         * @brief Gets the physical device associated with this logical device.
         *
         * @return Pointer to the PhysicalDevice object used to create this logical device.
         */
        [[nodiscard]] const PhysicalDevice& GetPhysicalDevice() const;

        /**
         * @brief Gets the cached indices of the queue families.
         *
         * @return QueueFamilyIndices struct containing the indices.
         */
        [[nodiscard]] QueueFamilyIndices GetQueueFamilyIndices() const;

        /**
         * @brief Blocks until all operations have finished.
         */
        void WaitUntilIdle() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Device(VulkanHelper::UniquePtr<Impl>&& impl);
    };
}