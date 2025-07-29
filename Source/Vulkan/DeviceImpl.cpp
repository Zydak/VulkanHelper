#include "Vulkan/Device.h"
#include "DeviceImpl.h"

#include "Log/Log.h"
#include "Vulkan/CommandPool.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "PhysicalDeviceImpl.h"

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Device::Impl>, VHResult> Device::Impl::New(const Config& config)
    {
        VulkanHelper::Vector<const char*> extensions;
        if (config.Window != nullptr)
        {
            extensions.EmplaceBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (!config.PhysicalDevice.IsSuitable(extensions))
        {
            VH_LOG_ERROR("Physical device is not suitable for the required extensions! Pick a different device.");
            return VulkanHelper::Unexpected(VHResult::EXTENSION_NOT_PRESENT);
        }
        
        // Create Queue Families
        QueueFamilyIndices indices = FindQueueFamilies(config.PhysicalDevice, config.Window);
        VulkanHelper::Vector<uint32_t> queueFamilyIndices;

        // Upload only unique queue families, sometimes the same family can be used for multiple operations 
        // (graphics queue with presentation capabilities is very common for example)
        if (indices.GraphicsFamily != UINT32_MAX)
            queueFamilyIndices.PushBack(indices.GraphicsFamily);
        if (indices.ComputeFamily != UINT32_MAX && indices.ComputeFamily != indices.GraphicsFamily)
            queueFamilyIndices.PushBack(indices.ComputeFamily);
        if (indices.PresentFamily != UINT32_MAX && indices.PresentFamily != indices.GraphicsFamily && indices.PresentFamily != indices.ComputeFamily)
            queueFamilyIndices.PushBack(indices.PresentFamily);

        VulkanHelper::Vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float priority = 1.0f;
        for (size_t i = 0; i < queueFamilyIndices.Size(); i++)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamilyIndices[i];
            queueCreateInfo.queueCount = 1; // Single queue per family for simplicity
            queueCreateInfo.pQueuePriorities = &priority;

            queueCreateInfos.PushBack(queueCreateInfo);
        }

        // Create logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.Size();
        createInfo.pQueueCreateInfos = queueCreateInfos.Data();
        createInfo.pEnabledFeatures = nullptr;
        createInfo.enabledExtensionCount = (uint32_t)extensions.Size();
        createInfo.ppEnabledExtensionNames = extensions.Data();
        createInfo.pNext = nullptr;

        #if !defined(NDEBUG)
        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
        #else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        #endif

        VkDevice device;
        VkResult res = vkCreateDevice(config.PhysicalDevice.m_Impl->GetDevice(), &createInfo, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create Vulkan device");
            return VulkanHelper::Unexpected(VHResult(res));
        }
        VH_LOG_INFO("Vulkan Device Implementation created successfully");

        return UniquePtr<Impl>(new Impl(config.PhysicalDevice, device, std::move(indices)));
    }

    Device::Impl::~Impl()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying device implementation");
            vkDestroyDevice(m_Device, nullptr);
            m_Device = nullptr;
        }
    }

    Device::Impl::Impl(Impl&& other) noexcept
        : m_PhysicalDevice(std::move(other.m_PhysicalDevice)), 
          m_Device(other.m_Device),
          m_QueueFamilyIndices(std::move(other.m_QueueFamilyIndices))
    {
        other.m_Device = nullptr;
    }

    Device::Impl& Device::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_PhysicalDevice = std::move(other.m_PhysicalDevice);
        m_Device = other.m_Device;
        m_QueueFamilyIndices = std::move(other.m_QueueFamilyIndices);

        other.m_Device = nullptr;

        return *this;
    }

    Device::QueueFamilyIndices Device::Impl::FindQueueFamilies(const PhysicalDevice& physicalDevice, const Window* window)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.m_Impl->GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return indices; // Will return with all indices as UINT32_MAX
        }

        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.m_Impl->GetDevice(), &queueFamilyCount, queueFamilies.Data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsFamily = i;

            // Check for dedicated compute queue
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                indices.ComputeFamily = i;

            // if window provided check for presentation support
            if (window != nullptr)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice.m_Impl->GetDevice(), i, window->GetSurface(), &presentSupport);
                if (presentSupport)
                    indices.PresentFamily = i;
            }
        }

        return indices;
    }

    //
    //  Forward functions
    //

    VulkanHelper::Expected<Device, VHResult> Device::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        VH_LOG_INFO("Creating Vulkan Device");

        return Device{ std::move(implResult.Value()) };
    }

    Device::Device(Device&& other) noexcept
        : m_Impl(std::move(other.m_Impl))
    {}

    Device& Device::operator=(Device&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Device(); // Clean up current state

        m_Impl = std::move(other.m_Impl);

        return *this;
    }

    Device::~Device()
    {
        VH_LOG_INFO("Destroying Vulkan Device");
    }

    const PhysicalDevice& Device::GetPhysicalDevice() const
    {
        return m_Impl->GetPhysicalDevice();
    }

    Device::QueueFamilyIndices Device::GetQueueFamilyIndices() const
    {
        return m_Impl->GetQueueFamilyIndices();
    }
} // namespace VulkanHelper