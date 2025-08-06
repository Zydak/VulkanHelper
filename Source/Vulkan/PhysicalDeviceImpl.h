#pragma once
#include "Vulkan/PhysicalDevice.h"

#include "Core/Expected.h"
#include "Core/Error.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class PhysicalDevice::Impl
    {
    public:

        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(VkInstance Instance, VkPhysicalDevice Device);

        Impl(const Impl& other);
        Impl& operator=(const Impl& other);

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const PhysicalDevice* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static PhysicalDevice CreatePublicInterface(VulkanHelper::UniquePtr<Impl>&& impl) { return PhysicalDevice(VulkanHelper::Move(impl)); }

        [[nodiscard]] bool IsSuitable(const VulkanHelper::Vector<const char*>& deviceExtensions) const;
        [[nodiscard]] inline VkPhysicalDevice GetDevice() const { return m_Device; }
        [[nodiscard]] inline Vendor GetVendor() const { return m_Vendor; }
        [[nodiscard]] inline const std::string& GetName() const { return m_Name; }
        [[nodiscard]] inline bool IsDiscrete() const { return m_Discrete; }

    private:

        Impl(VkPhysicalDevice device, Vendor vendor, std::string&& name, bool discrete)
            : m_Device(device), m_Vendor(vendor), m_Name(VulkanHelper::Move(name)), m_Discrete(discrete) {}

        VkPhysicalDevice m_Device;
        Vendor m_Vendor;
        std::string m_Name;
        bool m_Discrete;
    };
}