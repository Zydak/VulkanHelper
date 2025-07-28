#include "Vulkan/Device.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    class Device::Impl
    {
    public:

        [[nodiscard]] static VulkanHelper::Expected<Impl*, VHResult> New(const Config& config);
        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline VkDevice GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkQueue GetGraphicsQueue() const { return m_Queues.GraphicsQueue; }
        [[nodiscard]] inline VkQueue GetComputeQueue() const { return m_Queues.ComputeQueue; }
        [[nodiscard]] inline VkQueue GetPresentQueue() const { return m_Queues.PresentQueue; }
        [[nodiscard]] inline VkCommandPool GetGraphicsCommandPool() const { return m_CommandPools.GraphicsPool; }
        [[nodiscard]] inline VkCommandPool GetComputeCommandPool() const { return m_CommandPools.ComputePool; }
        [[nodiscard]] inline const PhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
    private:
        struct QueueFamilyIndices
        {
            uint32_t GraphicsFamily = UINT32_MAX;
            uint32_t ComputeFamily = UINT32_MAX;
            uint32_t PresentFamily = UINT32_MAX;
        };

        struct Queues
        {
            VkQueue GraphicsQueue = NULL;
            VkQueue ComputeQueue = NULL;
            VkQueue PresentQueue = NULL;
        };

        struct CommandPools
        {
            VkCommandPool GraphicsPool = NULL;
            VkCommandPool ComputePool = NULL;
        };

        Impl(PhysicalDevice physicalDevice, VkDevice device, Queues queues, CommandPools commandPools)
            : m_PhysicalDevice(physicalDevice), m_Device(device), m_Queues(std::move(queues)), m_CommandPools(std::move(commandPools)) {}

        PhysicalDevice m_PhysicalDevice;
        VkDevice m_Device = nullptr;
        Queues m_Queues;
        CommandPools m_CommandPools;
        
        [[nodiscard]] static QueueFamilyIndices FindQueueFamilies(const PhysicalDevice& physicalDevice, const Window* window);
    };

    VulkanHelper::Expected<Device::Impl*, VHResult> Device::Impl::New(const Config& config)
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
        VkResult res = vkCreateDevice(config.PhysicalDevice.GetDevice(), &createInfo, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create Vulkan device");
            return VulkanHelper::Unexpected(VHResult(res));
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
                return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
            }
        }
        
        if (indices.ComputeFamily != UINT32_MAX)
        {
            poolInfo.queueFamilyIndex = indices.ComputeFamily;
            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPools.ComputePool) != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create compute command pool");
                return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
            }
        }

        return new Impl(config.PhysicalDevice, device, std::move(queues), std::move(commandPools));
    }

    Device::Impl::~Impl()
    {
        if (m_Device != nullptr)
        {
            if (m_CommandPools.GraphicsPool != nullptr)
            {
                vkDestroyCommandPool(m_Device, m_CommandPools.GraphicsPool, nullptr);
                m_CommandPools.GraphicsPool = nullptr;
                VH_LOG_INFO("Destroying Graphics Command Pool");
            }
            if (m_CommandPools.ComputePool != nullptr)
            {
                vkDestroyCommandPool(m_Device, m_CommandPools.ComputePool, nullptr);
                m_CommandPools.ComputePool = nullptr;
                VH_LOG_INFO("Destroying Compute Command Pool");
            }
            vkDestroyDevice(m_Device, nullptr);
            m_Device = nullptr;
            VH_LOG_INFO("Destroying Vulkan Device");

            m_Queues = {};
        }
    }

    Device::Impl::Impl(Impl&& other) noexcept
        : m_PhysicalDevice(std::move(other.m_PhysicalDevice)), 
          m_Device(other.m_Device),
          m_Queues(other.m_Queues),
          m_CommandPools(other.m_CommandPools)
    {
        other.m_Device = nullptr;
        other.m_Queues = {};
        other.m_CommandPools = {};
    }

    Device::Impl& Device::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_PhysicalDevice = std::move(other.m_PhysicalDevice);
        m_Device = other.m_Device;
        m_Queues = std::move(other.m_Queues);
        m_CommandPools = std::move(other.m_CommandPools);

        other.m_Device = nullptr;
        other.m_Queues = {};
        other.m_CommandPools = {};

        return *this;
    }

    Device::Impl::QueueFamilyIndices Device::Impl::FindQueueFamilies(const PhysicalDevice& physicalDevice, const Window* window)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return indices; // Will return with all indices as UINT32_MAX
        }

        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice.GetDevice(), &queueFamilyCount, queueFamilies.Data());

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

        return Device{ implResult.Value() };
    }

    Device::Device(Device&& other) noexcept
        : m_Impl(other.m_Impl)
    {
        other.m_Impl = nullptr;
    }

    Device& Device::operator=(Device&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Device(); // Clean up current state

        m_Impl = other.m_Impl;
        other.m_Impl = nullptr;

        return *this;
    }

    Device::~Device()
    {
        if (m_Impl != nullptr)
        {
            delete m_Impl;
            m_Impl = nullptr;
            VH_LOG_INFO("Destroying Vulkan Device");
        }
    }

    VkDevice Device::GetDevice() const
    {
        return m_Impl->GetDevice();
    }

    VkQueue Device::GetGraphicsQueue() const
    {
        return m_Impl->GetGraphicsQueue();
    }

    VkQueue Device::GetComputeQueue() const
    {
        return m_Impl->GetComputeQueue();
    }

    VkQueue Device::GetPresentQueue() const
    {
        return m_Impl->GetPresentQueue();
    }

    VkCommandPool Device::GetGraphicsCommandPool() const
    {
        return m_Impl->GetGraphicsCommandPool();
    }

    VkCommandPool Device::GetComputeCommandPool() const
    {
        return m_Impl->GetComputeCommandPool();
    }

    const PhysicalDevice& Device::GetPhysicalDevice() const
    {
        return m_Impl->GetPhysicalDevice();
    }
} // namespace VulkanHelper