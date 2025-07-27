#include "Vulkan/PhysicalDevice.h"

#include <vulkan/vulkan.h>
#include "Log/Log.h"


namespace VulkanHelper
{
    std::expected<PhysicalDevice, VHResult> PhysicalDevice::New(const Config& config)
    {
        if (config.Device == nullptr || config.Instance == nullptr)
        {
            VH_LOG_ERROR("Invalid PhysicalDevice configuration: Device or Instance is null.");
            return std::unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(config.Device, &properties);

        Vendor vendor;
        switch (properties.vendorID)
        {
            case 0x10DE: // NVIDIA
                vendor = Vendor::NVIDIA;
                break;
            case 0x1002: // AMD
                vendor = Vendor::AMD;
                break;
            case 0x8086: // Intel
                vendor = Vendor::INTEL;
                break;
            case 0x1010: // ImgTec
                vendor = Vendor::ImgTec;
                break;
            case 0x13B5: // ARM
                vendor = Vendor::ARM;
                break;
            case 0x5143: // Qualcomm
                vendor = Vendor::Qualcomm;
                break;
            default:
                vendor = Vendor::Unknown;
                break;
        }

        bool discrete = (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

        return PhysicalDevice(config.Device, vendor, properties.deviceName, discrete);
    }

    PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept
        : m_Device(other.m_Device), m_Vendor(other.m_Vendor), m_Name(std::move(other.m_Name)), m_Discrete(other.m_Discrete)
    {
        other.m_Device = nullptr;
    }

    PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Device = other.m_Device;
        m_Vendor = other.m_Vendor;
        m_Name = std::move(other.m_Name);
        m_Discrete = other.m_Discrete;

        other.m_Device = nullptr;

        return *this;
    }

    bool PhysicalDevice::IsSuitable(const std::vector<const char*>& extensions) const
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, nullptr); // Get count of all available extensions
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, availableExtensions.data()); // Get all available extensions

        for (size_t i = 0; i < extensions.size(); i++)
        {
            bool found = false;
            for (size_t j = 0; j < availableExtensions.size(); j++)
            {
                if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                VH_LOG_WARN("Physical device {} does not support extension: {}", m_Name, extensions[i]);
                return false;
            }
        }

        return true;
    }
} // namespace VulkanHelper