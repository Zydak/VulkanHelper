#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Vector.h"

typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkInstance_T* VkInstance;

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

        struct Config
        {
            VkInstance Instance = nullptr;
            VkPhysicalDevice Device = nullptr;
        };

        /**
         * @brief Creates a new PhysicalDevice wrapper for a Vulkan physical device.
         *
         * This static factory function attempts to wrap a Vulkan VkPhysicalDevice handle with additional metadata and checks.
         * If successful, it returns a PhysicalDevice object; otherwise, it returns a VHError describing the failure.
         *
         * @param config The configuration struct specifying the Vulkan instance and physical device to wrap.
         * @return std::expected<PhysicalDevice, VHError> An expected containing the created PhysicalDevice on success, or a VHError on failure.
         */
        [[nodiscard]] static VulkanHelper::Expected<PhysicalDevice, VHResult> New(const Config& config);

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
         * @brief Retrieves the underlying Vulkan VkPhysicalDevice handle.
         *
         * @return VkPhysicalDevice The Vulkan physical device handle managed by this object.
         */
        [[nodiscard]] VkPhysicalDevice GetDevice() const;

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
        Impl* m_Impl;

        PhysicalDevice(Impl* impl)
            : m_Impl(impl)
        {}
    };
}