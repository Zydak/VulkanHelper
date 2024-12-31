#pragma once

#include "pch.h"

#include "vulkan/vulkan.h"

namespace VulkanHelper
{

	class Instance
	{
	public:

		struct CreateInfo
		{
			std::vector<int32_t> IgnoredMessageIDs;
		};

		static void Init(const CreateInfo& info);
		static void Destroy();

		inline VkInstance GetHandle() const { return m_Handle; }

		static Instance* Get();

	private:

		inline static Instance* s_Instance = nullptr;

		Instance() = default;
		~Instance() = default;

		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;
		Instance(Instance&& other) = delete;
		Instance& operator=(Instance&& other) = delete;

		static void PopulateDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		static std::vector<const char*> GetRequiredGlfwExtensions();
		static void CheckRequiredGlfwExtensions();

		void SetupDebugMessenger();

		void DestroyDebugUtilsMessengerEXT();
		void CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo);

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkInstance m_Handle = VK_NULL_HANDLE;
	};

}