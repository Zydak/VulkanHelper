#include "Core/Error.h"
#include "Vulkan/Device.h"
#include "DeviceImpl.h"

#include "Log/Log.h"
#include "Vulkan/CommandPool.h"
#include "Window/Window.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    Expected<SharedPtr<Device::Impl>, VHResult> Device::Impl::New(
        const SharedPtr<PhysicalDevice::Impl>& physicalDevice,
        const Vector<SharedPtr<Window::Impl>>& windows,
        const SharedPtr<Instance::Impl>& instance,
        bool requestRTSupport
    )
    {
        VH_LOG_INFO("Creating Vulkan Device Implementation");

        if (instance == nullptr)
        {
            VH_LOG_ERROR("Instance Pointer Can't be NULL!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<const char*> extensions;
        extensions.PushBack(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        extensions.PushBack(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        extensions.PushBack(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        if (requestRTSupport)
        {
            extensions.PushBack(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            extensions.PushBack(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            extensions.PushBack(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        }

        VkPhysicalDeviceFeatures2 features{};
        features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
        dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
        dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

        VkPhysicalDeviceVulkan11Features device11Features{};
        device11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        device11Features.shaderDrawParameters = true;

        VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features{};
        sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        sync2Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceBufferDeviceAddressFeaturesKHR bufferDeviceAddressFeatures{};
        bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
        bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

        VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutFeatures{};
        scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
        scalarBlockLayoutFeatures.scalarBlockLayout = VK_TRUE;

        features.pNext = &dynamicRenderingFeatures;
        dynamicRenderingFeatures.pNext = &device11Features;
        device11Features.pNext = &sync2Features;
        sync2Features.pNext = &bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.pNext = &scalarBlockLayoutFeatures;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{};
        VkPhysicalDeviceHostQueryResetFeatures hostQueryResetFeatures{};
        if (requestRTSupport)
        {
            rtFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            rtFeatures.rayTracingPipeline = VK_TRUE;
            scalarBlockLayoutFeatures.pNext = &rtFeatures;

            asFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            asFeatures.accelerationStructure = VK_TRUE;
            rtFeatures.pNext = &asFeatures;

            hostQueryResetFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES;
            hostQueryResetFeatures.hostQueryReset = VK_TRUE;
            asFeatures.pNext = &hostQueryResetFeatures;
        }

        if (!windows.Empty())
        {
            extensions.EmplaceBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        }

        if (!physicalDevice->IsSuitable(extensions))
        {
            VH_LOG_ERROR("Physical device is not suitable for the required device extensions! Pick a different device.");
            return VulkanHelper::Unexpected(VHResult::EXTENSION_NOT_PRESENT);
        }
        
        // Create Queue Families
        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice, windows);
        if (!windows.Empty() && indices.PresentFamily == UINT32_MAX)
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

        VkDevice device;
        VkResult res = vkCreateDevice(physicalDevice->GetDevice(), &createInfo, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create Vulkan device");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        VulkanMemoryAllocator allocator = VulkanMemoryAllocator::New(device, instance->GetInstance(), physicalDevice->GetDevice()).Value();

        VkPhysicalDeviceProperties2 physicalDeviceProperties = {};
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = {};
        rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
        rayTracingProperties.pNext = nullptr;
        VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties = {};
        accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
        accelerationStructureProperties.pNext = nullptr;
        if (requestRTSupport)
        {
            physicalDeviceProperties.pNext = &rayTracingProperties;
            rayTracingProperties.pNext = &accelerationStructureProperties;
        }

        vkGetPhysicalDeviceProperties2(physicalDevice->GetDevice(), &physicalDeviceProperties);

        // Create the Device::Impl first, then initialize the DeleteQueue with proper allocator pointer
        return SharedPtr<Impl>(new Impl(
            instance,
            device,
            physicalDevice,
            Move(indices),
            Move(allocator),
            Move(physicalDeviceProperties),
            Move(rayTracingProperties),
            Move(accelerationStructureProperties)
        ));
    }

    Device::Impl::~Impl()
    {
        if (m_Device != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying device implementation");

            // Flush all pending deletions before destroying the device
            m_DeleteQueue.Flush();

            m_Allocator.~VulkanMemoryAllocator(); // Allocator has to be destroyed before the device

            vkDestroyDevice(m_Device, nullptr);
            m_Device = nullptr;
        }
    }

    void Device::Impl::InitializeDeleteQueue(uint32_t framesDelay)
    {
        m_DeleteQueue = DeleteQueue(m_Device, &m_Allocator, framesDelay);
    }

    Device::Impl::Impl(Impl&& other) noexcept
        : m_Instance(other.m_Instance)
        , m_Device(other.m_Device)
        , m_PhysicalDevice(Move(other.m_PhysicalDevice))
        , m_QueueFamilyIndices(Move(other.m_QueueFamilyIndices))
        , m_Allocator(Move(other.m_Allocator))
        , m_DeleteQueue(Move(other.m_DeleteQueue))
        , m_PhysicalDeviceProperties(Move(other.m_PhysicalDeviceProperties))
        , m_RayTracingProperties(Move(other.m_RayTracingProperties))
        , m_AccelerationStructureProperties(Move(other.m_AccelerationStructureProperties))
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
        m_DeleteQueue = Move(other.m_DeleteQueue);
        m_PhysicalDeviceProperties = Move(other.m_PhysicalDeviceProperties);
        m_RayTracingProperties = Move(other.m_RayTracingProperties);
        m_AccelerationStructureProperties = Move(other.m_AccelerationStructureProperties);

        return *this;
    }

    Device::QueueFamilyIndices Device::Impl::FindQueueFamilies(const SharedPtr<PhysicalDevice::Impl>& physicalDevice, const VulkanHelper::Vector<SharedPtr<Window::Impl>>& windows)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice->GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return indices; // Will return with all indices as UINT32_MAX
        }

        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice->GetDevice(), &queueFamilyCount, queueFamilies.Data());

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
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice->GetDevice(), i, windows[j]->GetSurface(), &presentSupport);

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
    Device::Impl::AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowCpuAccess, uint32_t alignment)
    {
        return m_Allocator.AllocateBuffer(bufferInfo, allowCpuAccess, alignment);
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
        VulkanHelper::Vector<SharedPtr<Window::Impl>> windows;
        for (auto& window : config.Windows)
        {
            windows.PushBack(Window::Impl::GetImplementation(window));
        }

        auto implResult = Impl::New(
            PhysicalDevice::Impl::GetImplementation(config.PhysicalDevice),
            Move(windows),
            Instance::Impl::GetImplementation(config.Instance),
            config.RequestRTSupport
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        Device publicInterface = Impl::CreatePublicInterface(implResult.Value());

        publicInterface.m_Impl->InitializeDeleteQueue(3); // Initialize delete queue with 3 frames delay

        return publicInterface;
    }

    Device::Device()
        : m_Impl(nullptr)
    {
    }

    Device::Device(const Device& other)
        : m_Impl(other.m_Impl)
    {}

    Device& Device::operator=(const Device& other)
    {
        if (this != &other)
        {
            m_Impl = other.m_Impl;
        }
        return *this;
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

    Device::Device(const VulkanHelper::SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }

    Device::QueueFamilyIndices Device::GetQueueFamilyIndices() const
    {
        return m_Impl->GetQueueFamilyIndices();
    }

    void Device::WaitUntilIdle() const
    {
        m_Impl->WaitUntilIdle();
    }

    SampleCount Device::GetMaxSampleCount() const
    {
        VkSampleCountFlags sampleCounts = m_Impl->GetPhysicalDeviceProperties().properties.limits.framebufferColorSampleCounts & m_Impl->GetPhysicalDeviceProperties().properties.limits.framebufferDepthSampleCounts;
        
        // Sample counts are a bitmask, so we need to check which bits are set, otherwise we end up with an undefined mix of color and depth sample counts.
        return static_cast<SampleCount>(
            (sampleCounts & VK_SAMPLE_COUNT_64_BIT) ? SampleCount::COUNT_64_BIT :
            (sampleCounts & VK_SAMPLE_COUNT_32_BIT) ? SampleCount::COUNT_32_BIT :
            (sampleCounts & VK_SAMPLE_COUNT_16_BIT) ? SampleCount::COUNT_16_BIT :
            (sampleCounts & VK_SAMPLE_COUNT_8_BIT) ? SampleCount::COUNT_8_BIT :
            (sampleCounts & VK_SAMPLE_COUNT_4_BIT) ? SampleCount::COUNT_4_BIT :
            (sampleCounts & VK_SAMPLE_COUNT_2_BIT) ? SampleCount::COUNT_2_BIT :
            SampleCount::COUNT_1_BIT
        );
    }
} // namespace VulkanHelper