#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Vector.h"

typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkInstance_T* VkInstance;

namespace VulkanHelper
{
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
        [[nodiscard]] inline VkPhysicalDevice GetDevice() const { return m_Device; }

        /**
         * @brief Gets the vendor of the physical device.
         *
         * @return Vendor The vendor enum value representing the GPU manufacturer.
         */
        [[nodiscard]] inline Vendor GetVendor() const { return m_Vendor; }

        /**
         * @brief Gets the name of the physical device.
         *
         * @return const std::string& The name of the GPU as reported by the Vulkan driver.
         */
        [[nodiscard]] inline const std::string& GetName() const { return m_Name; }

        /**
         * @brief Checks if the physical device is a discrete GPU.
         *
         * @return true if the device is discrete (dedicated GPU); false if integrated or otherwise.
         */
        [[nodiscard]] inline bool IsDiscrete() const { return m_Discrete; }

    private:

        PhysicalDevice(VkPhysicalDevice device, Vendor vendor, std::string&& name, bool discrete)
            : m_Device(device), m_Vendor(vendor), m_Name(std::move(name)), m_Discrete(discrete) {}

        VkPhysicalDevice m_Device;
        Vendor m_Vendor;
        std::string m_Name;
        bool m_Discrete;
    };
}