#include "Core/Error.h"
#include "Vulkan/Device.h"
#include "DeviceImpl.h"

#include "Log/Log.h"
#include "Vulkan/CommandPool.h"
#include "Window/Window.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "PhysicalDeviceImpl.h"

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Device::Impl>, VHResult> Device::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Device Implementation");

        if (config.Instance == nullptr)
        {
            VH_LOG_ERROR("Instance Pointer Can't be NULL!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<const char*> extensions;
        extensions.PushBack(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

        VkPhysicalDeviceFeatures2 features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceVulkan11Features device11Features{};
        device11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        device11Features.shaderDrawParameters = true;

        features.pNext = &dynamicRenderingFeatures;
        dynamicRenderingFeatures.pNext = &device11Features;

        if (!config.Windows.Empty())
        {
            extensions.EmplaceBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (!config.PhysicalDevice.IsSuitable(extensions))
        {
            VH_LOG_ERROR("Physical device is not suitable for the required device extensions! Pick a different device.");
            return VulkanHelper::Unexpected(VHResult::EXTENSION_NOT_PRESENT);
        }
        
        // Create Queue Families
        QueueFamilyIndices indices = FindQueueFamilies(config.PhysicalDevice, config.Windows);
        if (!config.Windows.Empty() && indices.PresentFamily == UINT32_MAX)
        {
            VH_LOG_ERROR("Presentation requested but not supported!");
            return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

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
        createInfo.pNext = &features;

        #if !defined(NDEBUG)
        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
        #else
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        #endif

        PhysicalDevice::Impl* physicalDeviceImpl = PhysicalDevice::Impl::GetImplementation(&config.PhysicalDevice);
        VkDevice device;
        VkResult res = vkCreateDevice(physicalDeviceImpl->GetDevice(), &createInfo, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create Vulkan device");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        Instance::Impl* instanceImpl = Instance::Impl::GetImplementation(config.Instance);
        VulkanMemoryAllocator allocator = VulkanMemoryAllocator::New({device, instanceImpl->GetInstance(), physicalDeviceImpl->GetDevice()}).Value();

        return UniquePtr<Impl>(new Impl(instanceImpl, device, Move(config.PhysicalDevice), Move(indices), Move(allocator)));
    }

    Device::Impl::~Impl()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying device implementation");

            m_Allocator.~VulkanMemoryAllocator(); // Allocator has to be destroyed before the device

            vkDestroyDevice(m_Device, nullptr);
            m_Device = nullptr;
        }
    }

    Device::Impl::Impl(Impl&& other) noexcept
        : m_Instance(other.m_Instance)
        , m_Device(other.m_Device)
        , m_PhysicalDevice(Move(other.m_PhysicalDevice))
        , m_QueueFamilyIndices(Move(other.m_QueueFamilyIndices))
        , m_Allocator(Move(other.m_Allocator))
    {
        other.m_Device = nullptr;
    }

    Device::Impl& Device::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_Instance = other.m_Instance;
        other.m_Instance = nullptr;
        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_PhysicalDevice = Move(other.m_PhysicalDevice);
        m_QueueFamilyIndices = Move(other.m_QueueFamilyIndices);
        m_Allocator = Move(other.m_Allocator);

        return *this;
    }

    Device::QueueFamilyIndices Device::Impl::FindQueueFamilies(const PhysicalDevice& physicalDevice, const VulkanHelper::Vector<Window*>& windows)
    {
        QueueFamilyIndices indices;

        PhysicalDevice::Impl* physicalDeviceImpl = PhysicalDevice::Impl::GetImplementation(&physicalDevice);

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceImpl->GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return indices; // Will return with all indices as UINT32_MAX
        }

        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceImpl->GetDevice(), &queueFamilyCount, queueFamilies.Data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsFamily = i;

            // Check for dedicated compute queue
            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
                indices.ComputeFamily = i;

            if (!windows.Empty())
            {
                VkBool32 presentSupport = false;
                for (size_t j = 0; j < windows.Size(); j++)
                {
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDeviceImpl->GetDevice(), i, windows[j]->GetSurface(), &presentSupport);
                    
                    // If even one listed window's surface isn't supported, break the loop and mark presentation support as false
                    if (presentSupport == false)
                        break;
                }

                if (presentSupport)
                    indices.PresentFamily = i;
            }
        }

        return indices;
    }

    void Device::Impl::WaitUntilIdle() const
    {
        vkDeviceWaitIdle(m_Device);
    }

    VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> 
    Device::Impl::AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowCpuAccess)
    {
        return m_Allocator.AllocateBuffer(bufferInfo, allowCpuAccess);
    }

    VulkanHelper::Expected<VulkanMemoryAllocator::ImageAllocation, VHResult> 
    Device::Impl::AllocateImage(const VkImageCreateInfo& imageInfo, bool allowMapping)
    {
        return m_Allocator.AllocateImage(imageInfo, allowMapping);
    }

    void Device::Impl::DeallocateBuffer(const VulkanMemoryAllocator::BufferAllocation& allocation)
    {
        m_Allocator.DeallocateBuffer(allocation);
    }

    void Device::Impl::DeallocateImage(const VulkanMemoryAllocator::ImageAllocation& allocation)
    {
        m_Allocator.DeallocateImage(allocation);
    }

    VulkanHelper::Expected<void*, VHResult> Device::Impl::MapMemory(const VmaAllocation& allocation)
    {
        return m_Allocator.MapMemory(allocation);
    }

    void Device::Impl::UnmapMemory(const VmaAllocation& allocation)
    {
        m_Allocator.UnmapMemory(allocation);
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

        return Device{ VulkanHelper::Move(implResult.Value()) };
    }

    Device::Device(Device&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Device& Device::operator=(Device&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Device(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Device::~Device()
    {

    }

    Device::Device(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    const PhysicalDevice& Device::GetPhysicalDevice() const
    {
        return m_Impl->GetPhysicalDevice();
    }

    Device::QueueFamilyIndices Device::GetQueueFamilyIndices() const
    {
        return m_Impl->GetQueueFamilyIndices();
    }

    void Device::WaitUntilIdle() const
    {
        m_Impl->WaitUntilIdle();
    }
} // namespace VulkanHelper