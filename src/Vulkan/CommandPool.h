#pragma once
#include "Pch.h"
#include "Instance.h"

namespace VulkanHelper
{
	class CommandPool
	{
	public:
		struct CreateInfo
		{
			VkDevice Device;
			uint32_t QueueFamilyIndex;
			VkCommandPoolCreateFlags Flags;
		};

		[[nodiscard]] VkResult Init(const CreateInfo& createInfo);
		void Destroy();

		CommandPool() = default;
		CommandPool(const CreateInfo& createInfo);
		~CommandPool();
		CommandPool(const CommandPool&) = delete;
		CommandPool& operator=(const CommandPool&) = delete;
		CommandPool(CommandPool&& other) noexcept;
		CommandPool& operator=(CommandPool&& other) noexcept;

		[[nodiscard]] VkCommandPool GetHandle() const { return m_Handle; }
		[[nodiscard]] uint32_t GetQueueFamilyIndex() const { return m_QueueFamilyIndex; }
		[[nodiscard]] VkCommandPoolCreateFlags GetFlags() const { return m_Flags; }
		[[nodiscard]] bool IsInitialized() const { return m_Initialized; }

		void ResetCommandPool() const;
	private:

		VkCommandPool m_Handle = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		uint32_t m_QueueFamilyIndex = 0;
		VkCommandPoolCreateFlags m_Flags = 0;

		bool m_Initialized = false;
		void Move(CommandPool&& other) noexcept;
		void Reset();
	};
}

