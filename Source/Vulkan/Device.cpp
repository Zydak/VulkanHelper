#include "Vulkan/Device.h"
#include "Log/Log.h"

#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    std::expected<Device, VHResult> Device::New(const Config& config)
    {
        std::vector<const char*> extensions;
        if (config.Window != nullptr)
        {
            extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (!config.PhysicalDevice.IsSuitable(extensions))
        {
            VH_LOG_ERROR("Physical device is not suitable for the required extensions! Pick a different device.");
            return std::unexpected(VHResult::EXTENSION_NOT_PRESENT);
        }
        
        // Create Queue Families
        QueueFamilyIndices indices = FindQueueFamilies(config.PhysicalDevice, config.Window);
        std::vector<uint32_t> queueFamilyIndices;

        // Upload only unique queue families, sometimes the same family can be used for multiple operations 
        // (graphics queue with presentation capabilities is very common for example)
        if (indices.GraphicsFamily != UINT32_MAX)
            queueFamilyIndices.push_back(indices.GraphicsFamily);
        if (indices.ComputeFamily != UINT32_MAX && indices.ComputeFamily != indices.GraphicsFamily)
            queueFamilyIndices.push_back(indices.ComputeFamily);
        if (indices.PresentFamily != UINT32_MAX && indices.PresentFamily != indices.GraphicsFamily && indices.PresentFamily != indices.ComputeFamily)
            queueFamilyIndices.push_back(indices.PresentFamily);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float priority = 1.0f;
        for (uint32_t familyIndex : queueFamilyIndices)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = familyIndex;
            queueCreateInfo.queueCount = 1; // Single queue per family for simplicity
            queueCreateInfo.pQueuePriorities = &priority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Create logical device
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr;
        createInfo.enabledExtensionCount = (uint32_t)extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();
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
        VkResult res = vkCreateDevice(config.PhysicalDevice.GetDevice(), &createInfo, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create Vulkan device");
            return std::unexpected(VHResult(res));
        }
        VH_LOG_INFO("Vulkan Device created successfully");

        Queues queues;
        if (indices.GraphicsFamily != UINT32_MAX)
            vkGetDeviceQueue(device, indices.GraphicsFamily, 0, &queues.GraphicsQueue);

        if (indices.ComputeFamily != UINT32_MAX)
            vkGetDeviceQueue(device, indices.ComputeFamily, 0, &queues.ComputeQueue);

        if (indices.PresentFamily != UINT32_MAX)
            vkGetDeviceQueue(device, indices.PresentFamily, 0, &queues.PresentQueue);

        // Create command pools
        CommandPools commandPools;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = indices.GraphicsFamily;

        if (indices.GraphicsFamily != UINT32_MAX)
        {
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPools.GraphicsPool) != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create graphics command pool");
                return std::unexpected(VHResult::INITIALIZATION_FAILED);
            }
        }
        
        if (indices.ComputeFamily != UINT32_MAX)
        {
            poolInfo.queueFamilyIndex = indices.ComputeFamily;
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPools.ComputePool) != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create compute command pool");
                return std::unexpected(VHResult::INITIALIZATION_FAILED);
            }
        }

        return Device(device, std::move(queues), std::move(commandPools));
    }

    Device::~Device()
    {
        if (m_Device != nullptr)
        {
            if (m_CommandPools.GraphicsPool != nullptr)
            {
                vkDestroyCommandPool(m_Device, m_CommandPools.GraphicsPool, nullptr);
                VH_LOG_INFO("Destroying Graphics Command Pool");
            }
            if (m_CommandPools.ComputePool != nullptr)
            {
                vkDestroyCommandPool(m_Device, m_CommandPools.ComputePool, nullptr);
                VH_LOG_INFO("Destroying Compute Command Pool");
            }
            vkDestroyDevice(m_Device, nullptr);
            VH_LOG_INFO("Destroying Vulkan Device");
        }
    }

    Device::Device(Device&& other) noexcept
        : m_Device(other.m_Device),
          m_Queues(other.m_Queues),
          m_CommandPools(other.m_CommandPools)
    {
        other.m_Device = nullptr;
        other.m_Queues = {};
        other.m_CommandPools = {};
    }

    Device& Device::operator=(Device&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Device = other.m_Device;
        m_Queues = std::move(other.m_Queues);
        m_CommandPools = std::move(other.m_CommandPools);

        other.m_Device = nullptr;
        other.m_Queues = {};
        other.m_CommandPools = {};

        return *this;
    }

    Device::QueueFamilyIndices Device::FindQueueFamilies(const PhysicalDevice& physicalDevice, Window* window)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return indices; // Will return with all indices as UINT32_MAX
        }

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.GetDevice(), &queueFamilyCount, queueFamilies.data());

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
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice.GetDevice(), i, window->GetSurface(), &presentSupport);
                if (presentSupport)
                    indices.PresentFamily = i;
            }
        }

        return indices;
    }
} // namespace VulkanHelper