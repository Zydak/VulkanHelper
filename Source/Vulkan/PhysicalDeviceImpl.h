#pragma once
#include "Vulkan/PhysicalDevice.h"

#include "Core/Expected.h"
#include "Core/Error.h"

typedef struct VkPhysicalDevice_T* VkPhysicalDevice;
typedef struct VkInstance_T* VkInstance;

namespace VulkanHelper
{
    class PhysicalDevice::Impl
    {
    public:
        struct Config
        {
            VkInstance Instance = nullptr;
            VkPhysicalDevice Device = nullptr;
        };

        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Config& config);

        Impl(const Impl& other);
        Impl& operator=(const Impl& other);

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] bool IsSuitable(const VulkanHelper::Vector<const char*>& extensions) const;
        [[nodiscard]] inline VkPhysicalDevice GetDevice() const { return m_Device; }
        [[nodiscard]] inline Vendor GetVendor() const { return m_Vendor; }
        [[nodiscard]] inline const std::string& GetName() const { return m_Name; }
        [[nodiscard]] inline bool IsDiscrete() const { return m_Discrete; }

    private:

        Impl(VkPhysicalDevice device, Vendor vendor, std::string&& name, bool discrete)
            : m_Device(device), m_Vendor(vendor), m_Name(std::move(name)), m_Discrete(discrete) {}

        VkPhysicalDevice m_Device;
        Vendor m_Vendor;
        std::string m_Name;
        bool m_Discrete;
    };
}