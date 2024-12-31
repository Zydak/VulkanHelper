#include "pch.h"

#include "Instance.h"

#include "vulkan.hpp"
#include "GLFW/glfw3.h"

#include "Asset/AssetManager.h"
#include "Vulkan/DeleteQueue.h"

#include "Scene/Components.h"
#include "Asset/Serializer.h"
#include "Audio/Audio.h"

namespace VulkanHelper
{

	static std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	static inline std::vector<int32_t> s_IgnoredMessageIDs;

	void Instance::Init(const CreateInfo& info)
	{
		if (s_Instance != nullptr)
			Destroy(); // Reinitialize

		glfwInit();

		Logger::Init();
		VK_CORE_TRACE("LOGGER INITIALIZED");
		Audio::Init();
		VK_CORE_TRACE("AUDIO INITIALIZED");

		REGISTER_CLASS_IN_SERIALIZER(ScriptComponent);
		REGISTER_CLASS_IN_SERIALIZER(TransformComponent);
		REGISTER_CLASS_IN_SERIALIZER(NameComponent);
		REGISTER_CLASS_IN_SERIALIZER(MeshComponent);
		REGISTER_CLASS_IN_SERIALIZER(MaterialComponent);
		REGISTER_CLASS_IN_SERIALIZER(TonemapperSettingsComponent);
		REGISTER_CLASS_IN_SERIALIZER(BloomSettingsComponent);

		s_IgnoredMessageIDs = std::move(info.IgnoredMessageIDs);

		s_Instance = new Instance();

		// Set up application information
		vk::ApplicationInfo appInfo;
		appInfo.pApplicationName = "VulkanHelper";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_2;

		// Set up instance creation information
		vk::InstanceCreateInfo createInfo;
		createInfo.pApplicationInfo = &appInfo;

		// Debug messenger creation info (if validation layers enabled)
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
#ifndef DISTRIBUTION
		{
			// Enable validation layers
			createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();

			// Populate debug messenger creation info
			PopulateDebugMessenger(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
#else
		{
			// Disable validation layers
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}
#endif

		// Check if required GLFW extensions are supported
		CheckRequiredGlfwExtensions();

		// Get required GLFW extensions
		std::vector<const char*> glfwExtensions = GetRequiredGlfwExtensions();
		// Set enabled extension count and extension names
		createInfo.enabledExtensionCount = (uint32_t)glfwExtensions.size();
		createInfo.ppEnabledExtensionNames = glfwExtensions.data();

		// Create Vulkan instance
		s_Instance->m_Handle = vk::createInstance(createInfo);

		// Create debug messenger if validation layers are enabled
#ifndef DISTRIBUTION
		s_Instance->SetupDebugMessenger();
#endif
	}

	void Instance::Destroy()
	{
		VK_CORE_ASSERT(s_Instance != nullptr, "Instance is not initialized!");

		Audio::Destroy();

		// Destroy debug messenger if validation layers are enabled
#ifndef DISTRIBUTION
		s_Instance->DestroyDebugUtilsMessengerEXT();
		s_Instance->m_DebugMessenger = VK_NULL_HANDLE;
#endif

		// Destroy Vulkan instance
		vkDestroyInstance(Instance::Get()->GetHandle(), nullptr);
		s_Instance->m_Handle = VK_NULL_HANDLE;

		delete s_Instance;
	}

	Instance* Instance::Get()
	{
		VK_CORE_ASSERT(s_Instance != nullptr, "Instance is not initialized!");

		return s_Instance;
	}

	std::vector<const char*> Instance::GetRequiredGlfwExtensions()
	{
		// Get the count and list of required GLFW extensions
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		// Convert the array of extension names to a vector
		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		// If validation layers are enabled, add the debug utils extension to the list
#ifndef DISTRIBUTION
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
#endif

		// Return the vector containing the required GLFW extensions
		return extensions;
	}

	/*
	 * @brief Queries the available Vulkan instance extension properties and verifies that
	 * all the required extensions for GLFW integration are present.
	 */
	void Instance::CheckRequiredGlfwExtensions()
	{
		// Query the number of available instance extension properties
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		// Retrieve the available instance extension properties
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		// Create an unordered set to store available extension names for fast lookup
		std::unordered_set<std::string> available;
		for (const auto& extension : extensions)
		{
			available.insert(extension.extensionName);
		}

		// Get the list of required GLFW extensions
		auto requiredExtensions = GetRequiredGlfwExtensions();

		// Iterate over each required extension and check if it is supported
		for (const auto& required : requiredExtensions)
		{
			// If the required extension is not found in the available set, raise an assertion
			if (available.find(required) == available.end())
			{
				VK_CORE_ASSERT(false, "Missing glfw extension: {0}", required);
			}
		}
	}

	void Instance::SetupDebugMessenger()
	{
		// Create the debug messenger creation info struct and populate it
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateDebugMessenger(createInfo);

		CreateDebugUtilsMessengerEXT(&createInfo);
	}

	/*
	   *  @brief Callback function for Vulkan to be called by validation layers when needed
	*/
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
			VK_CORE_INFO("Info: {0} - {1} : {2}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			//VL_CORE_ERROR("");

			VK_CORE_ERROR("Error\n\t{0} - {1} : {2}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			//VL_CORE_WARN("");

			VK_CORE_WARN("Warning\n\t{0} - {1} : {2}", pCallbackData->messageIdNumber, pCallbackData->pMessageIdName, pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

	void Instance::PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		// Initialize the createInfo struct
		createInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

		// Specify the severity levels of messages to be handled by the callback
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

		// Specify the types of messages to be handled by the callback
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

		// Specify the callback function to be called when a debug message is generated
		createInfo.pfnUserCallback = DebugCallback;
	}

	/*
	 * @brief Loads vkDestroyDebugUtilsMessengerEXT from memory and then calls it.
	 * This is necessary because this function is an extension function, it is not automatically loaded to memory
	 */
	void Instance::DestroyDebugUtilsMessengerEXT()
	{
		static auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handle, "vkDestroyDebugUtilsMessengerEXT");
		VK_CORE_ASSERT(func != nullptr, "Can't load vkDestroyDebugUtilsMessengerEXT!");
		func(m_Handle, m_DebugMessenger, nullptr);
	}


	/*
	 * @brief Loads vkCreateDebugUtilsMessengerEXT from memory and then calls it.
	 * This is necessary because this function is an extension function, it is not automatically loaded to memory
	 */
	void Instance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo)
	{
		static auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handle, "vkDestroyDebugUtilsMessengerEXT");
		VK_CORE_ASSERT(func != nullptr, "Can't load vkCreateDebugUtilsMessengerEXT!");
		func(m_Handle, pCreateInfo, nullptr, &m_DebugMessenger);
	}
}