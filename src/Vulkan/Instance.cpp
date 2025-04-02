#include "Pch.h"
#include "Instance.h"

#include "GLFW/glfw3.h"
#include "Logger/Logger.h"

static inline std::vector<int32_t> s_IgnoredMessageIDs;

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	for (int i = 0; i < s_IgnoredMessageIDs.size(); i++)
	{
		if (pCallbackData->messageIdNumber == s_IgnoredMessageIDs[i])
			return VK_FALSE;
	}

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		VH_INFO("Info: {0} - {1} : {2}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		VH_ERROR("Error\n\t{0} - {1} : {2}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		VH_WARN("Warning\n\t{0} - {1} : {2}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
	}
	return VK_FALSE;
}

void VulkanHelper::Instance::Init(const CreateInfo& createInfo)
{
	if (s_Instance != nullptr)
		Destroy();

	s_Instance = new Instance();

	VulkanHelper::Logger::Init();
	VulkanHelper::Logger::GetInstance()->SetLevel(spdlog::level::trace);

	glfwInit();
	VH_INFO("GLFW initialized");

	s_IgnoredMessageIDs = createInfo.IgnoredMessageIDs;

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

// Enable debug layers when not in distribution
#ifndef DISTRIBUTION
	const char* debugLayer = "VK_LAYER_KHRONOS_validation";
	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = &debugLayer;

	VkDebugUtilsMessengerCreateInfoEXT debugLayersCreateInfo{};
	debugLayersCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugLayersCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	debugLayersCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugLayersCreateInfo.pfnUserCallback = DebugCallback;
	instanceCreateInfo.pNext = &debugLayersCreateInfo;
#endif

	CheckRequiredGlfwExtensionsPresence();

	std::vector<const char*> glfwExtensions = GetRequiredGlfwExtensions();
	instanceCreateInfo.enabledExtensionCount = (uint32_t)glfwExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = glfwExtensions.data();

	VH_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &s_Instance->m_Handle) == VK_SUCCESS, "Couldn't create instance");

#ifndef DISTRIBUTION
	s_Instance->CreateDebugUtilsMessengerEXT(&debugLayersCreateInfo);
#endif
}

void VulkanHelper::Instance::Destroy()
{
	VH_ASSERT(s_Instance != nullptr, "Instance is not initialized!");

#ifndef DISTRIBUTION
	s_Instance->DestroyDebugUtilsMessengerEXT();
#endif

	vkDestroyInstance(s_Instance->m_Handle, nullptr);

	delete s_Instance;
	VH_INFO("Instance destroyed!");

	Logger::Destroy();
}

std::vector<const char*> VulkanHelper::Instance::GetRequiredGlfwExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifndef DISTRIBUTION
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	return extensions;
}

void VulkanHelper::Instance::CheckRequiredGlfwExtensionsPresence()
{
	// Query the number of available instance extension properties
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	// Retrieve the available instance extension properties
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	std::unordered_set<std::string> available;
	for (const auto& extension : extensions)
	{
		available.insert(extension.extensionName);
	}

	auto requiredExtensions = GetRequiredGlfwExtensions();

	for (const auto& required : requiredExtensions)
	{
		if (available.find(required) == available.end())
		{
			VH_ASSERT(false, "Missing glfw extension: {0}", required);
		}
	}
}

std::vector<VulkanHelper::Instance::PhysicalDevice> VulkanHelper::Instance::QuerySuitablePhysicalDevices(VkSurfaceKHR surface, const std::vector<const char*> extensions)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Handle, &deviceCount, nullptr); // Get device count
	VH_ASSERT(deviceCount != 0, "failed to find GPUs with Vulkan support!");
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Handle, &deviceCount, devices.data());

	std::vector<VulkanHelper::Instance::PhysicalDevice> outDevices;

	for (uint32_t i = 0; i < deviceCount; i++)
	{
		if (!IsDeviceSuitable(devices[i], surface, extensions))
		{
			continue;
		}

		outDevices.resize(i + 1);
		outDevices[i].Handle = devices[i];

		if (surface != VK_NULL_HANDLE)
		{
			outDevices[i].SwapchainSupport = QuerySwapchainSupport(devices[i], surface);
		}

		outDevices[i].Properties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		vkGetPhysicalDeviceProperties2(outDevices[i].Handle, &outDevices[i].Properties);

		{
			// Retrieve and store properties of the selected physical device for later use
			VkPhysicalDeviceProperties2 properties2{};
			properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(devices[i], &properties2);

			// Determine the vendor
			switch (properties2.properties.vendorID)
			{
			case 0x1002:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::AMD;
				break;

			case 0x10DE:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::NVIDIA;
				break;

			case 0x8086:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::INTEL;
				break;

			case 0x1010:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::ImgTec;
				break;

			case 0x13B5:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::ARM;
				break;

			case 0x5143:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::Qualcomm;
				break;

			default:
				outDevices[i].Vendor = Instance::PhysicalDevice::Vendor::Unknown;
				break;
			}

			outDevices[i].Name = properties2.properties.deviceName;

			if (properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				outDevices[i].Discrete = true;
			}
		}
	}

	return outDevices;
}

bool VulkanHelper::Instance::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> extensions) const
{
	if (!AreExtensionsSupported(device, extensions))
	{
		return false;
	}
	if (!AreQueueFamiliesSupported(device, surface))
	{
		return false;
	}

	if (surface != VK_NULL_HANDLE)
	{
		// Check if it supports anything in the swapchain
		PhysicalDevice::SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(device, surface);
		if (swapchainSupport.Formats.empty() || swapchainSupport.PresentModes.empty())
		{
			return false;
		}
	}

	return true;
}

bool VulkanHelper::Instance::AreExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*> extensions) const
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr); // Get count of all available extensions
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()); // Get all available extensions

	for (size_t i = 0; i < extensions.size(); i++)
	{
		bool found = false;
		for (size_t j = 0; j < availableExtensions.size(); j++)
		{
			if (strcmp(extensions[i], availableExtensions[j].extensionName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			return false;
		}
	}

	return true;
}

bool VulkanHelper::Instance::AreQueueFamiliesSupported(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
	PhysicalDevice::QueueFamilyIndices indices = FindQueueFamilies(device, surface);
	return indices.IsComplete();
}

VulkanHelper::Instance::PhysicalDevice::QueueFamilyIndices VulkanHelper::Instance::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
	PhysicalDevice::QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr); // Get queue family count
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data()); // Get queue families

	if (surface == VK_NULL_HANDLE)
	{
		indices.NeedsPresentFamily = false;
	}

	int i = 0;
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		// Check if the queue family supports both graphics and compute operations
		if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			indices.GraphicsFamily = i;
			indices.GraphicsFamilyHasValue = true;
		}
		// Check if the queue family supports only compute operations
		if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			indices.ComputeFamily = i;
			indices.ComputeFamilyHasValue = true;
		}

		if (indices.NeedsPresentFamily)
		{
			// Check if the queue family supports presentation to the associated surface
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.PresentFamily = i;
				indices.PresentFamilyHasValue = true;
			}
		}

		if (indices.IsComplete()) // If all found return
		{
			break;
		}
	}

	return indices;
}

VulkanHelper::Instance::PhysicalDevice::SwapchainSupportDetails VulkanHelper::Instance::QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
	PhysicalDevice::SwapchainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
	}

	return details;
}

// ------------------------------------------------------------------------------------------------
// Loaded Functions

void VulkanHelper::Instance::DestroyDebugUtilsMessengerEXT() const
{
	static auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handle, "vkDestroyDebugUtilsMessengerEXT");
	VH_ASSERT(func != nullptr, "Can't load vkDestroyDebugUtilsMessengerEXT!");
	func(m_Handle, m_DebugMessenger, nullptr);
}

void VulkanHelper::Instance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo)
{
	static auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handle, "vkCreateDebugUtilsMessengerEXT");
	VH_ASSERT(func != nullptr, "Can't load vkCreateDebugUtilsMessengerEXT!");
	func(m_Handle, pCreateInfo, nullptr, &m_DebugMessenger);
}

// ------------------------------------------------------------------------------------------------

VkFormat VulkanHelper::Instance::PhysicalDevice::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(Handle, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) { return format; }
	}

	// Return VK_FORMAT_UNDEFINED if no supported format is found among the candidates
	return VK_FORMAT_UNDEFINED;
}
