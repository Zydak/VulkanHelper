#include "Vulkan/Instance.h"
#include "Core/Error.h"
#include "vulkan/vulkan.h"
#include "Log/Log.h"
#include <expected>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "GLFW/glfw3.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
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

namespace VulkanHelper
{
    std::expected<Instance, VHError> Instance::New(const Instance::Config& config)
    {
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

        std::vector<const char*> extensions;

        if (config.AddGLFWExtensions)
        {
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            for (uint32_t i = 0; i < glfwExtensionCount; ++i)
            {
                extensions.emplace_back(glfwExtensions[i]);
            }
        }

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

            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instanceCreateInfo.enabledExtensionCount = extensions.size();
            instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
        #endif

        VkInstance instance;
        VkResult res = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
        if (res != VK_SUCCESS)
        {
            return std::unexpected(VHError(res)); // VkResult maps to VHError so this is legal
        }
        
        VkDebugUtilsMessengerEXT messenger;
        Instance::CreateDebugUtilsMessengerEXT(instance, &debugLayersCreateInfo, &messenger);

        return Instance(messenger, instance);
    }

    Instance::Instance(Instance&& other) noexcept
        : m_DebugMessenger(other.m_DebugMessenger), m_Instance(other.m_Instance)
    {
        other.m_DebugMessenger = nullptr;
        other.m_Instance = nullptr;
    }

    Instance& Instance::operator=(Instance&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_DebugMessenger = other.m_DebugMessenger;
        other.m_DebugMessenger = nullptr;

        m_Instance = other.m_Instance;
        other.m_Instance = nullptr;

        return *this;
    }

    Instance::~Instance()
    {
        if (m_Instance != nullptr)
        {
            if (m_DebugMessenger != nullptr)
                Instance::DestroyDebugUtilsMessengerEXT(m_Instance, &m_DebugMessenger);

            vkDestroyInstance(m_Instance, nullptr);
        }
    }

    std::vector<PhysicalDevice> Instance::GetSuitablePhysicalDevices(const std::vector<const char*>& extensions) const
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            VH_LOG_ERROR("No Vulkan physical devices found!");
            return {};
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        std::vector<PhysicalDevice> suitableDevices;
        suitableDevices.reserve(deviceCount);
        for (const auto& device : devices)
        {
            auto physicalDevice = PhysicalDevice::New({ m_Instance, device });
            if (physicalDevice.has_value() && physicalDevice.value().IsSuitable(extensions))
            {
                suitableDevices.emplace_back(std::move(physicalDevice.value()));
            }
            else
            {
                VH_LOG_WARN("Physical device {} is not suitable for the requested extensions.", physicalDevice->GetName());
            }
        }

        return suitableDevices;
    }

    void Instance::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* outDebugMessenger)
    {
        static auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        VH_ASSERT(func != nullptr, "Can't load vkCreateDebugUtilsMessengerEXT!");
        func(instance, pCreateInfo, nullptr, outDebugMessenger);
    }
    
    void Instance::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger)
    {
        static auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        VH_ASSERT(func != nullptr, "Can't load vkDestroyDebugUtilsMessengerEXT!");
        func(instance, *debugMessenger, nullptr);
    }
}