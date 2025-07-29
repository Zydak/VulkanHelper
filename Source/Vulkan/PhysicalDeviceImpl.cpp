#include "Vulkan/PhysicalDevice.h"
#include "PhysicalDeviceImpl.h"

#include "Core/Expected.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<PhysicalDevice::Impl>, VHResult> PhysicalDevice::Impl::New(const Config& config)
    {
        if (config.Device == nullptr || config.Instance == nullptr)
        {
            VH_LOG_ERROR("Invalid PhysicalDevice configuration: Device or Instance is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
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

        return VulkanHelper::UniquePtr(new Impl(config.Device, vendor, properties.deviceName, discrete));
    }

    PhysicalDevice::Impl::Impl(const Impl& other)
        : m_Device(other.m_Device), m_Vendor(other.m_Vendor), m_Name(other.m_Name), m_Discrete(other.m_Discrete)
    {

    }

    PhysicalDevice::Impl& PhysicalDevice::Impl::operator=(const Impl& other)
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_Device = other.m_Device;
        m_Vendor = other.m_Vendor;
        m_Name = other.m_Name;
        m_Discrete = other.m_Discrete;

        return *this;
    }

    PhysicalDevice::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device), m_Vendor(other.m_Vendor), m_Name(std::move(other.m_Name)), m_Discrete(other.m_Discrete)
    {
        other.m_Device = nullptr;
    }

    PhysicalDevice::Impl& PhysicalDevice::Impl::operator=(Impl&& other) noexcept
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

    bool PhysicalDevice::Impl::IsSuitable(const VulkanHelper::Vector<const char*>& extensions) const
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, nullptr); // Get count of all available extensions
        VulkanHelper::Vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_Device, nullptr, &extensionCount, availableExtensions.Data()); // Get all available extensions

        for (size_t i = 0; i < extensions.Size(); i++)
        {
            bool found = false;
            for (size_t j = 0; j < availableExtensions.Size(); j++)
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

    //
    //  Forward functions
    //

    PhysicalDevice::PhysicalDevice(PhysicalDevice&& other) noexcept
        : m_Impl(std::move(other.m_Impl))
    {}

    PhysicalDevice& PhysicalDevice::operator=(PhysicalDevice&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~PhysicalDevice(); // Clean up current state

        m_Impl = std::move(other.m_Impl);

        return *this;
    }

    PhysicalDevice::PhysicalDevice(const PhysicalDevice& other)
        : m_Impl(new Impl(*other.m_Impl)) // Deep copy
    {
    }

    PhysicalDevice& PhysicalDevice::operator=(const PhysicalDevice& other)
    {
        if (this == &other)
            return *this;

        this->~PhysicalDevice(); // Clean up current state

        m_Impl = new Impl(*other.m_Impl); // Deep copy

        return *this;
    }

    PhysicalDevice::~PhysicalDevice()
    {
        
    }

    bool PhysicalDevice::IsSuitable(const VulkanHelper::Vector<const char*>& extensions) const
    {
        return m_Impl->IsSuitable(extensions);
    }

    PhysicalDevice::Vendor PhysicalDevice::GetVendor() const
    {
        return m_Impl->GetVendor();
    }

    const char* PhysicalDevice::GetName() const
    {
        return m_Impl->GetName().c_str();
    }

    bool PhysicalDevice::IsDiscrete() const
    {
        return m_Impl->IsDiscrete();
    }
} // namespace VulkanHelper