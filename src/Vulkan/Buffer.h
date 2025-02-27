#pragma once
#include "Pch.h"

#include "Device.h"
#include <vk_mem_alloc.h>

namespace VulkanHelper
{
	class Buffer
	{
	public:
		struct CreateInfo
		{
			Device* Device = nullptr;
			VkDeviceSize BufferSize = 0;
			VkMemoryPropertyFlags MemoryPropertyFlags = 0;
			VkBufferUsageFlags UsageFlags = 0;
			bool DedicatedAllocation = true;
		};

		Buffer(const Buffer::CreateInfo& createInfo);
		~Buffer();

		Buffer(const Buffer& other) = delete;
		Buffer& operator=(const Buffer& other) = delete;
		Buffer(Buffer&& other) noexcept;
		Buffer& operator=(Buffer&& other) noexcept;
		
	public:

		[[nodiscard]] VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void Unmap();

		void Barrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkCommandBuffer cmd = VK_NULL_HANDLE);

		void WriteToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0, VkCommandBuffer cmdBuffer = 0);
		void ReadFromBuffer(void* outData, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0, VkCommandBuffer cmdBuffer = 0);

		[[nodiscard]] VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		[[nodiscard]] VkDescriptorBufferInfo DescriptorInfo();

		[[nodiscard]] inline VkBuffer GetHandle() const { return m_Handle; }

		[[nodiscard]] inline void* GetMappedMemory() const { return m_Mapped; }
		[[nodiscard]] VmaAllocationInfo GetMemoryInfo() const;
		[[nodiscard]] VkDeviceAddress GetDeviceAddress() const;
		[[nodiscard]] inline bool IsMapped() const { return m_Mapped; }

		[[nodiscard]] inline VkBufferUsageFlags GetUsageFlags() const { return m_UsageFlags; }
		[[nodiscard]] inline VkMemoryPropertyFlags GetMemoryPropertyFlags() const { return m_MemoryPropertyFlags; }
		[[nodiscard]] inline VkDeviceSize GetBufferSize() const { return m_BufferSize; }
		[[nodiscard]] inline bool IsDedicatedAllocation() const { return m_IsDedicatedAllocation; }
		[[nodiscard]] inline VmaAllocation* GetAllocation() { return m_Allocation; }

	private:

		Device* m_Device = nullptr;

		VkBuffer m_Handle = VK_NULL_HANDLE;
		VmaAllocation* m_Allocation = nullptr;

		void* m_Mapped = nullptr;
		VkDeviceSize m_BufferSize = 0;
		VkBufferUsageFlags m_UsageFlags = 0;
		VkMemoryPropertyFlags m_MemoryPropertyFlags = 0;
		bool m_IsDedicatedAllocation = false;

		void Destroy();
		void Move(Buffer&& other);
	};
}
