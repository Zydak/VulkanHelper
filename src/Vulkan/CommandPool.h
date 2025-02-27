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

		CommandPool(const CreateInfo& createInfo);
		~CommandPool();

		CommandPool(const CommandPool&) = delete;
		CommandPool& operator=(const CommandPool&) = delete;
		CommandPool(CommandPool&& other) noexcept;
		CommandPool& operator=(CommandPool&& other) noexcept;
	public:

		void ResetCommandPool() const;

	public:

		[[nodiscard]] VkCommandPool GetHandle() const { return m_Handle; }
		[[nodiscard]] uint32_t GetQueueFamilyIndex() const { return m_QueueFamilyIndex; }
		[[nodiscard]] VkCommandPoolCreateFlags GetFlags() const { return m_Flags; }

	private:

		VkCommandPool m_Handle = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		uint32_t m_QueueFamilyIndex = 0;
		VkCommandPoolCreateFlags m_Flags = 0;

		void Destroy();
		void Move(CommandPool&& other) noexcept;
	};
}

