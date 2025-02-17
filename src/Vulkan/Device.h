#pragma once
#include "Pch.h"
#include "Instance.h"

#include "CommandPool.h"

namespace VulkanHelper
{

	class Device
	{
	public:
		struct CreateInfo
		{
			std::vector<const char*> Extensions;
			VkPhysicalDeviceFeatures2 Features;
			Instance::PhysicalDevice PhysicalDevice;
			VkSurfaceKHR Surface;
		};

		struct CommandPools
		{
			CommandPool Graphics;
			CommandPool Compute;
		};

		void Init(const CreateInfo& createInfo);
		void Destroy();

		Device() = default;
		Device(const CreateInfo& createInfo);
		~Device();

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		Device(Device&& other) noexcept;
		Device& operator=(Device&& other) noexcept;

		[[nodiscard]] VkDevice GetHandle() const { return m_Handle; }
		[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		[[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
		[[nodiscard]] VkQueue GetComputeQueue() const { return m_ComputeQueue; }
		[[nodiscard]] bool IsInitialized() const { return m_Initialized; }

		void CreateCommandPoolsForThread();
	private:

		void CreateLogicalDevice();

		VkDevice m_Handle = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		Instance::PhysicalDevice m_PhysicalDevice;
		std::vector<const char*> m_Extensions;
		VkPhysicalDeviceFeatures2 m_Features;

		std::unordered_map<std::thread::id, CommandPools> m_CommandPools;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;

		bool m_Initialized = false;
		void Move(Device&& other) noexcept;
		void Reset();
	};

}