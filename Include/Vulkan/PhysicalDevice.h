#pragma once

#include "Core/Vector.h"
#include "Core/UniquePtr.h"
#include "Core/Macros.h"

namespace VulkanHelper
{
    /**
     * @class PhysicalDevice
     * @brief RAII wrapper for a Vulkan physical device.
     *
     * Manages the lifetime of a Vulkan physical device, providing functionality to check suitability.
     */
    class PhysicalDevice
    {
    public:
        enum class Vendor
        {
            NVIDIA,
			AMD,
			INTEL,
			ImgTec,
			ARM,
			Qualcomm,
			Unknown
        };

        ~PhysicalDevice();

        PhysicalDevice(const PhysicalDevice& other);
        PhysicalDevice& operator=(const PhysicalDevice& other);

        PhysicalDevice(PhysicalDevice&& other) noexcept;
        PhysicalDevice& operator=(PhysicalDevice&& other) noexcept;

        /**
         * @brief Checks if the physical device supports the required extensions and is suitable for use.
         *
         * This function evaluates whether the physical device meets the application's requirements by checking for the presence of the specified Vulkan extensions.
         *
         * @param extensions A vector of required Vulkan extension names.
         * @return true if the device supports all required extensions and is suitable; false otherwise.
         */
        [[nodiscard]] bool IsSuitable(const VulkanHelper::Vector<const char*>& extensions) const;

        /**
         * @brief Gets the vendor of the physical device.
         *
         * @return Vendor The vendor enum value representing the GPU manufacturer.
         */
        [[nodiscard]] Vendor GetVendor() const;

        /**
         * @brief Gets the name of the physical device.
         *
         * @return const char* The name of the GPU as reported by the Vulkan driver.
         */
        [[nodiscard]] const char* GetName() const;

        /**
         * @brief Checks if the physical device is a discrete GPU.
         *
         * @return true if the device is discrete (dedicated GPU); false if integrated or otherwise.
         */
        [[nodiscard]] bool IsDiscrete() const;

    private:
        class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        PhysicalDevice(VulkanHelper::UniquePtr<Impl>&& impl);

        #undef PHYSICAL_DEVICE_CLASS
        DECLARE_FRIENDS();
        #define PHYSICAL_DEVICE_CLASS PhysicalDevice
    };
}