#pragma once

#include "Core/Vector.h"
#include "Core/UniquePtr.h"

namespace VulkanHelper
{
    /**
     * @class PhysicalDevice
     * @brief RAII wrapper for a physical GPU device. Provides GPU capabilities querying and device suitability checking.
     */
    class PhysicalDevice
    {
    public:
        /**
         * @brief GPU vendor identifiers
         */
        enum class Vendor
        {
            NVIDIA,    ///< NVIDIA GPU
            AMD,       ///< AMD GPU
            INTEL,     ///< Intel GPU
            ImgTec,    ///< Imagination Technologies GPU
            ARM,       ///< ARM GPU
            Qualcomm,  ///< Qualcomm GPU
            Unknown    ///< Unknown vendor
        };

        ~PhysicalDevice();

        PhysicalDevice(const PhysicalDevice& other);
        PhysicalDevice& operator=(const PhysicalDevice& other);

        PhysicalDevice(PhysicalDevice&& other) noexcept;
        PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;

        /**
         * @brief Checks if the device supports the specified Vulkan Device Extensions. Returns true if all required deviceExtensions are supported.
         * 
         * @param deviceExtensions List of required Vulkan device extension names
         * @return true if device is suitable, false otherwise
         * @note Device must have Vulkan driver installed
         */
        [[nodiscard]] bool IsSuitable(const VulkanHelper::Vector<const char*>& deviceExtensions) const;

        /**
         * @brief Gets the GPU manufacturer identifier.
         * 
         * @return Vendor The GPU vendor enum value
         */
        [[nodiscard]] Vendor GetVendor() const;

        /**
         * @brief Gets the GPU device name as reported by the driver.
         * 
         * @return const char* The GPU device name
         */
        [[nodiscard]] const char* GetName() const;

        /**
         * @brief Checks if this is a discrete (dedicated) GPU.
         * 
         * @return true if discrete GPU, false if integrated
         */
        [[nodiscard]] bool IsDiscrete() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        /**
         * @brief Internal constructor used by Instance class
         */
        PhysicalDevice(VulkanHelper::UniquePtr<Impl>&& impl);

        friend class Instance; // Allow Instance to construct PhysicalDevice
    };
}