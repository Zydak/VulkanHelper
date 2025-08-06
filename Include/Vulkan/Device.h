#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Vulkan/PhysicalDevice.h"
#include "Instance.h"

namespace VulkanHelper
{
    class Window;
    
    /**
     * @class Device
     * @brief RAII wrapper for Vulkan logical device objects
     * 
     * Manages queues and device-level Vulkan operations.
     */
    class Device
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a logical device
         */
        struct Config
        {
            /**
             * @brief Physical device to create the logical device from
             * @note Must be a valid device that supports required features
             */
            VulkanHelper::PhysicalDevice PhysicalDevice;

            /**
             * @brief Windows that the device will present to
             * @note If not empty, device will create presentation-capable queues
             */
            VulkanHelper::Vector<VulkanHelper::Window*> Windows;

            /**
             * @brief Vulkan instance to create the device from
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Instance* Instance = nullptr;
        };

        /**
         * @struct QueueFamilyIndices
         * @brief Queue family indices for different types of operations
         */
        struct QueueFamilyIndices
        {
            uint32_t GraphicsFamily = UINT32_MAX;  ///< Index for graphics operations
            uint32_t ComputeFamily = UINT32_MAX;   ///< Index for compute operations
            uint32_t PresentFamily = UINT32_MAX;   ///< Index for presentation operations
        };

        /**
         * @brief Creates a new logical device
         * @param config Configuration parameters for the device
         * @return Expected containing the created device or an error code
         */
        [[nodiscard]] static VulkanHelper::Expected<Device, VHResult> New(const Config& config);
        /**
         * @brief Destructor
         */
        ~Device();

        /**
         * @brief Delete copy constructor
         */
        Device(const Device& other) = delete;

        /**
         * @brief Delete copy assignment operator
         */
        Device& operator=(const Device& other) = delete;

        /**
         * @brief Move constructor
         */
        Device(Device&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        Device& operator=(Device&& other) noexcept;

        /**
         * @brief Get queue family indices for different operations
         * @return Queue family indices used by this device
         */
        [[nodiscard]] QueueFamilyIndices GetQueueFamilyIndices() const;

        /**
         * @brief Wait for all device operations to complete
         */
        void WaitUntilIdle() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Device(VulkanHelper::UniquePtr<Impl>&& impl);
    };
}