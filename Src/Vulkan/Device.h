#pragma once
#include "Pch.h"
#include "Instance.h"

#include "CommandPool.h"
#include "vk_mem_alloc.h"
#include "ErrorCodes.h"

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
		Device(const CreateInfo& createInfo) { Init(createInfo); }
		Device() = default;
		~Device();

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;
		Device(Device&&) noexcept = delete;
		Device& operator=(Device&&) noexcept = delete;

	public:

		inline void WaitUntilIdle() const { vkDeviceWaitIdle(m_Handle); }

		void CreateCommandPoolsForThread();

		void SetObjectName(VkObjectType type, uint64_t handle, const char* name) const;

		[[nodiscard]] VkResult FindMemoryTypeIndex(uint32_t* outMemoryIndex, VkMemoryPropertyFlags flags) const;

		[[nodiscard]] ResultCode AllocateBuffer(VkBuffer* outBuffer, VmaAllocation* outAllocation, const VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags memoryPropertyFlags = 0, bool dedicatedAllocation = false);
		[[nodiscard]] ResultCode AllocateImage(VkImage* outImage, VmaAllocation* outAllocation, const VkImageCreateInfo& createInfo, VkMemoryPropertyFlags memoryPropertyFlags = 0, bool dedicatedAllocation = true);

		void BeginSingleTimeCommands(VkCommandBuffer* buffer, VkCommandPool pool);
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool);

	public:

		[[nodiscard]] VkDevice GetHandle() const { return m_Handle; }
		[[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		[[nodiscard]] VkQueue GetPresentQueue() const { return m_PresentQueue; }
		[[nodiscard]] VkQueue GetComputeQueue() const { return m_ComputeQueue; }
		[[nodiscard]] std::mutex* GetGraphicsQueueMutex() { return &m_GraphicsQueueMutex; }
		[[nodiscard]] std::mutex* GetComputeQueueMutex() { return &m_ComputeQueueMutex; }
		[[nodiscard]] CommandPool* GetGraphicsCommandPool() { return &m_CommandPools[std::this_thread::get_id()]->Graphics; }
		[[nodiscard]] CommandPool* GetComputeCommandPool() { return &m_CommandPools[std::this_thread::get_id()]->Compute; }
		[[nodiscard]] Instance::PhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		[[nodiscard]] VmaAllocator GetAllocator() const { return m_Allocator; }

	private:

		void CreateLogicalDevice();
		void CreateMemoryAllocator();

		VkDevice m_Handle = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		Instance::PhysicalDevice m_PhysicalDevice;
		std::vector<const char*> m_Extensions;
		VkPhysicalDeviceFeatures2 m_Features;

		std::unordered_map<std::thread::id, std::unique_ptr<CommandPools>> m_CommandPools;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		std::mutex m_GraphicsQueueMutex;
		VkQueue m_ComputeQueue = VK_NULL_HANDLE;
		std::mutex m_ComputeQueueMutex;

		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		void Destroy();
	};

}