#pragma once
#include "pch.h"

#include "VulkanHelper/Utility/Utility.h"
#include "Vulkan/Device.h"

namespace VulkanHelper
{
	struct WindowInfo
	{
		uint32_t WindowWidth = 600;
		uint32_t WindowHeight = 600;
		std::string Name = "";
		std::string Icon = "";
		std::string WorkingDirectory = "";
	};

	struct QueryDevicesInfo
	{
		std::shared_ptr<Window> Window;
		bool EnableRayTracingSupport = false;
		bool UseMemoryAddress = true;
		std::vector<const char*> DeviceExtensions;
		std::vector<const char*> OptionalExtensions;
		VkPhysicalDeviceFeatures2 Features = VkPhysicalDeviceFeatures2();
		std::vector<int32_t> IgnoredMessageIDs;
	};

	struct InitializationInfo
	{
		std::shared_ptr<Window> Window;
		VulkanHelper::Device::PhysicalDevice PhysicalDevice;
		uint32_t MaxFramesInFlight = 2;
	};

	std::shared_ptr<Window> InitWindow(const WindowInfo& windowInfo);
	std::vector<Device::PhysicalDevice> QueryDevices(const QueryDevicesInfo& queryInfo);
	void Init(const InitializationInfo& initInfo);
	void EndFrame();
	void Destroy();

}