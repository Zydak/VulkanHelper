#include "Core/UniquePtr.h"
#include "Vulkan/Instance.h"
#include "InstanceImpl.h"

#include "Core/Error.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "PhysicalDeviceImpl.h"
#include "Vulkan/PhysicalDevice.h"

#include "FunctionLoader.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void*)
{
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		VH_LOG_INFO("Info: {} - {} : {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		VH_LOG_ERROR("Error\n\t{} - {} : {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		VH_LOG_WARN("Warning\n\t{} - {} : {}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static void GLFWErrorCallback(int errorCode, const char* message)
{
    VH_LOG_ERROR("GLFW ERROR: Error code: {} | Message: {}", errorCode, message);
}

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Instance::Impl>, VHResult> Instance::Impl::New(const Instance::Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Instance Implementation");

        static bool GLFWInitialized = false;
        if (GLFWInitialized == false && config.AddGLFWExtensions)
        {
            if (glfwInit() != GLFW_TRUE)
                return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);

            glfwSetErrorCallback(GLFWErrorCallback);

            GLFWInitialized = true;
        }

        VkApplicationInfo appInfo
        {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = "VulkanHelper",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "VulkanHelper",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_2
        };

        VkInstanceCreateInfo instanceCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
        };

        VulkanHelper::Vector<const char*> extensions;

        if (config.AddGLFWExtensions)
        {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            if (glfwExtensions == nullptr)
            {
                VH_LOG_ERROR("Failed to get GLFW required instance extensions!");
                return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
            }

            extensions.Reserve(glfwExtensionCount + 1);
            for (uint32_t i = 0; i < glfwExtensionCount; ++i)
            {
                extensions.EmplaceBack(glfwExtensions[i]);
            }
        }

        // Layers only enabled in debug builds
        #if !defined(NDEBUG)
            const char* debugLayer = "VK_LAYER_KHRONOS_validation";
            instanceCreateInfo.enabledLayerCount = 1;
            instanceCreateInfo.ppEnabledLayerNames = &debugLayer;

            VkDebugUtilsMessengerCreateInfoEXT debugLayersCreateInfo{};
            debugLayersCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugLayersCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
            debugLayersCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugLayersCreateInfo.pfnUserCallback = DebugCallback;
            instanceCreateInfo.pNext = &debugLayersCreateInfo;

            extensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instanceCreateInfo.enabledExtensionCount = (uint32_t)extensions.Size();
            instanceCreateInfo.ppEnabledExtensionNames = extensions.Data();
        #endif

        for (size_t i = 0; i < extensions.Size(); i++)
        {
            VH_LOG_DEBUG("Enabling Vulkan Extension: {}", extensions[i]);
        }

        VkInstance instance;
        VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
        if (res != VK_SUCCESS)
            return VulkanHelper::Unexpected(VHResult(res)); // VkResult maps to VHError so this is legal

        FunctionLoader::SetInstance(instance);
        
        #if !defined(NDEBUG)
        VkDebugUtilsMessengerEXT messenger;
        Impl::CreateDebugUtilsMessengerEXT(instance, &debugLayersCreateInfo, &messenger);
        #endif

        return UniquePtr<Impl>(new Impl(messenger, instance));
    }

    Instance::Impl::Impl(Impl&& other) noexcept
        : m_DebugMessenger(other.m_DebugMessenger), m_Instance(other.m_Instance)
    {
        other.m_DebugMessenger = nullptr;
        other.m_Instance = nullptr;
    }

    Instance::Impl& Instance::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_DebugMessenger = other.m_DebugMessenger;
        other.m_DebugMessenger = nullptr;

        m_Instance = other.m_Instance;
        other.m_Instance = nullptr;

        return *this;
    }

    Instance::Impl::~Impl()
    {
        if (m_Instance != nullptr)
        {
            VH_LOG_INFO("Destroying Vulkan Instance Implementation");
            if (m_DebugMessenger != nullptr)
            {
                Impl::DestroyDebugUtilsMessengerEXT(m_Instance, &m_DebugMessenger);
                m_DebugMessenger = nullptr;
            }

            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = nullptr;
        }
    }

    VulkanHelper::Vector<PhysicalDevice> Instance::Impl::GetSuitablePhysicalDevices() const
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        VulkanHelper::Vector<const char*> deviceExtensions;
        deviceExtensions.PushBack(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

        if (deviceCount == 0)
        {
            VH_LOG_ERROR("No Vulkan physical devices found!");
            return {};
        }

        VulkanHelper::Vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.Data());

        VulkanHelper::Vector<PhysicalDevice> suitableDevices;
        suitableDevices.Reserve(deviceCount);
        for (size_t i = 0; i < devices.Size(); i++)
        {
            auto physicalDeviceImpl = PhysicalDevice::Impl::New({m_Instance, devices[i]});
            if (physicalDeviceImpl.HasValue() && physicalDeviceImpl.Value()->IsSuitable(deviceExtensions))
            {
                PhysicalDevice physicalDevice(VulkanHelper::Move(physicalDeviceImpl.Value()));
                suitableDevices.EmplaceBack(VulkanHelper::Move(physicalDevice));
            }
            else
            {
                VkPhysicalDeviceProperties properties;
                vkGetPhysicalDeviceProperties(devices[i], &properties);
                VH_LOG_WARN("Physical device {} is not suitable for the requested extensions.", properties.deviceName);
            }
        }

        return suitableDevices;
    }

    void Instance::Impl::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* outDebugMessenger)
    {
        static auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        VH_ASSERT(func != nullptr, "Can't load vkCreateDebugUtilsMessengerEXT!");
        func(instance, pCreateInfo, nullptr, outDebugMessenger);
    }
    
    void Instance::Impl::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger)
    {
        static auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        VH_ASSERT(func != nullptr, "Can't load vkDestroyDebugUtilsMessengerEXT!");
        func(instance, *debugMessenger, nullptr);
    }

    //
    //  Forward functions
    //

    VulkanHelper::Expected<Instance, VHResult> Instance::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Instance{ VulkanHelper::Move(implResult.Value()) };
    }

    Instance::Instance(Instance&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {
        other.m_Impl = nullptr;
    }

    Instance& Instance::operator=(Instance&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Instance(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Instance::~Instance()
    {

    }

    Instance::Instance(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    VulkanHelper::Vector<PhysicalDevice> Instance::GetSuitablePhysicalDevices() const
    {
        return m_Impl->GetSuitablePhysicalDevices();
    }

    VkInstance Instance::GetInstance() const
    {
        return m_Impl->GetInstance();
    }
}