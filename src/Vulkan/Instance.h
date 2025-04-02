#pragma once
#include "Pch.h"

#include "vulkan/vulkan.hpp"

namespace VulkanHelper
{

	class Instance
	{
	public:
		struct CreateInfo
		{
			std::vector<int32_t> IgnoredMessageIDs;
		};

		struct PhysicalDevice 
		{
			enum class Vendor
			{
				NVIDIA,
				AMD,
				INTEL,
				ImgTec,
				ARM,
				Qualcomm,
				Unknown
			};

			struct QueueFamilyIndices
			{
				uint32_t GraphicsFamily = 0;
				uint32_t PresentFamily = 0;
				uint32_t ComputeFamily = 0;
				bool GraphicsFamilyHasValue = false;
				bool PresentFamilyHasValue = false;
				bool ComputeFamilyHasValue = false;

				bool NeedsPresentFamily = true;

				[[nodiscard]] inline bool IsComplete() const { return GraphicsFamilyHasValue && ComputeFamilyHasValue && (!NeedsPresentFamily || PresentFamilyHasValue); }
			};

			struct SwapchainSupportDetails
			{
				VkSurfaceCapabilitiesKHR Capabilities{};      // min/max number of images
				std::vector<VkSurfaceFormatKHR> Formats;    // pixel format, color space
				std::vector<VkPresentModeKHR> PresentModes;
			};

			VkPhysicalDeviceProperties2 Properties;
			SwapchainSupportDetails SwapchainSupport;
			std::string Name = "INVALID DEVICE";
			Vendor Vendor = Vendor::Unknown;
			bool Discrete = false;
			VkPhysicalDevice Handle = VK_NULL_HANDLE;

			[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
			[[nodiscard]] inline SwapchainSupportDetails QuerySwapchainSupport(VkSurfaceKHR surface) { SwapchainSupport = Instance::Get()->QuerySwapchainSupport(Handle, surface); return SwapchainSupport; }
		};

		static void Init(const CreateInfo& createInfo);
		static void Destroy();

	public:
		[[nodiscard]] std::vector<PhysicalDevice> QuerySuitablePhysicalDevices(VkSurfaceKHR surface, const std::vector<const char*> extensions);
		[[nodiscard]] PhysicalDevice::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
		[[nodiscard]] PhysicalDevice::SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;

	public:

		[[nodiscard]] inline static Instance* Get() { return s_Instance; }

		[[nodiscard]] inline VkInstance GetHandle() const { return m_Handle; }

	private:
		Instance() = default;
		~Instance() = default;

		Instance(const Instance&) = delete;
		Instance(const Instance&&) = delete;
		Instance& operator=(const Instance&) = delete;
		Instance& operator=(Instance&&) = delete;

		[[nodiscard]] bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> extensions) const;
		[[nodiscard]] bool AreExtensionsSupported(VkPhysicalDevice device, const std::vector<const char*> extensions) const;
		[[nodiscard]] bool AreQueueFamiliesSupported(VkPhysicalDevice device, VkSurfaceKHR surface) const;
		
		[[nodiscard]] static std::vector<const char*> GetRequiredGlfwExtensions();
		static void CheckRequiredGlfwExtensionsPresence();

		inline static Instance* s_Instance = nullptr;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkInstance m_Handle = VK_NULL_HANDLE;

		// Loaded functions
		void DestroyDebugUtilsMessengerEXT() const;
		void CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo);
	};

} // namespace VulkanHelper