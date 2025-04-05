#include "Pch.h"

#include "Buffer.h"
#include "Logger/Logger.h"

#include "Device.h"
#include "DeleteQueue.h"

namespace VulkanHelper
{
	ResultCode Buffer::Init(const Buffer::CreateInfo& createInfo)
	{
		if (m_Handle != VK_NULL_HANDLE)
			Destroy();

		m_Allocation = new VmaAllocation();

		m_Device = createInfo.Device;
		m_UsageFlags = createInfo.UsageFlags;
		m_MemoryPropertyFlags = createInfo.MemoryPropertyFlags;
		m_IsDedicatedAllocation = createInfo.DedicatedAllocation;

		m_BufferSize = createInfo.BufferSize;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = m_BufferSize;
		bufferInfo.usage = m_UsageFlags;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		return (ResultCode)m_Device->AllocateBuffer(&m_Handle, m_Allocation, bufferInfo, m_MemoryPropertyFlags, createInfo.DedicatedAllocation);
	}

	void Buffer::Destroy()
	{
		Unmap();

		DeleteQueue::DeleteBuffer(*this);

		Reset();
	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		if (this == &other)
			return;

		if (m_Handle != VK_NULL_HANDLE)
			Destroy();

		Move(std::move(other));
	}

	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
		if (this == &other)
			return *this;

		if (m_Handle != VK_NULL_HANDLE)
			Destroy();

		Move(std::move(other));

		return *this;
	}

	Buffer::~Buffer()
	{
		if (m_Handle != VK_NULL_HANDLE)
			Destroy();
	}

	VmaAllocationInfo Buffer::GetMemoryInfo() const
	{
		VmaAllocationInfo info{};
		vmaGetAllocationInfo(m_Device->GetAllocator(), *m_Allocation, &info);

		return info;
	}

	VkDeviceAddress Buffer::GetDeviceAddress() const
	{
		VkBufferDeviceAddressInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		info.buffer = m_Handle;

		return vkGetBufferDeviceAddress(m_Device->GetHandle(), &info);
	}

	ResultCode Buffer::Map(VkDeviceSize size, VkDeviceSize offset)
	{
		VH_ASSERT(!(m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "Can't map device local buffer!");

		VkResult result = vmaMapMemory(m_Device->GetAllocator(), *m_Allocation, &m_Mapped);

		return (ResultCode)result;
	}

	void Buffer::Unmap()
	{
		if (m_Mapped)
		{
			vmaUnmapMemory(m_Device->GetAllocator(), *m_Allocation);
			m_Mapped = nullptr;
		}
	}

	void Buffer::Barrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkCommandBuffer cmd)
	{
		bool cmdProvided = cmd != VK_NULL_HANDLE;

		if (!cmdProvided)
		{
			m_Device->BeginSingleTimeCommands(&cmd, m_Device->GetGraphicsCommandPool()->GetHandle());
		}

		VkBufferMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstAccessMask = dstAccess;
		barrier.srcAccessMask = srcAccess;
		barrier.offset = 0;
		barrier.size = VK_WHOLE_SIZE;
		barrier.buffer = m_Handle;

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 1, &barrier, 0, nullptr);

		if (!cmdProvided)
		{
			m_Device->EndSingleTimeCommands(cmd, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());
		}
	}

	ResultCode Buffer::WriteToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset, VkCommandBuffer cmdBuffer)
	{
		VH_ASSERT((size == VK_WHOLE_SIZE || size + offset <= m_BufferSize), "Data size is larger than buffer size, either resize the buffer or create a larger one");
		VH_ASSERT(data != nullptr, "Invalid data pointer");

		if (size == VK_WHOLE_SIZE)
			size = m_BufferSize - offset;

		ResultCode res = ResultCode::Success;

		// If the buffer is device local, use a staging buffer to transfer the data.
		if (m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			VkCommandBuffer cmd;
			if (cmdBuffer == VK_NULL_HANDLE)
				m_Device->BeginSingleTimeCommands(&cmd, m_Device->GetGraphicsCommandPool()->GetHandle());
			else
				cmd = cmdBuffer;

			Buffer::CreateInfo info{};
			info.BufferSize = size;
			info.Device = m_Device;
			info.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			info.UsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			Buffer stagingBuffer;
			res = stagingBuffer.Init(info);
			if (res != ResultCode::Success)
				return res;

			res = stagingBuffer.Map();
			if (res != ResultCode::Success)
				return res;

			stagingBuffer.WriteToBuffer(data, size, 0, cmd);
			res = stagingBuffer.Flush();
			if (res != ResultCode::Success)
				return res;

			stagingBuffer.Unmap();

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = offset;
			copyRegion.size = size;

			vkCmdCopyBuffer(cmd, stagingBuffer.GetHandle(), m_Handle, 1, &copyRegion);

			if (cmdBuffer == VK_NULL_HANDLE)
				m_Device->EndSingleTimeCommands(cmd, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());
		}
		else // If the buffer is not device local, write directly to the buffer.
		{
			VH_ASSERT(m_Mapped, "Cannot copy to unmapped buffer");

			char* memOffset = reinterpret_cast<char*>(m_Mapped) + offset;

			memcpy(memOffset, data, size);
		}

		return res;
	}

	void Buffer::ReadFromBuffer(void* outData, VkDeviceSize size /*= VK_WHOLE_SIZE*/, VkDeviceSize offset /*= 0*/, VkCommandBuffer cmdBuffer /*= 0*/)
	{
		VH_ASSERT((size == VK_WHOLE_SIZE || size + offset <= m_BufferSize), "Data size is larger than buffer size on reading!");
		VH_ASSERT(outData != nullptr, "Invalid outData pointer");

		if (size == VK_WHOLE_SIZE)
			size = m_BufferSize - offset;

		if (m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			VH_ASSERT((m_UsageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) != 0, "Can't read from buffer that is DEVICE_LOCAL and wasn't created with USAGE_TRANSFERS_SRC_BIT!");

			VkCommandBuffer cmd;
			if (cmdBuffer == VK_NULL_HANDLE)
				m_Device->BeginSingleTimeCommands(&cmd, m_Device->GetGraphicsCommandPool()->GetHandle());
			else
				cmd = cmdBuffer;

			Buffer::CreateInfo info{};
			info.Device = m_Device;
			info.BufferSize = size;
			info.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			info.UsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			Buffer stagingBuffer;
			VH_CHECK(stagingBuffer.Init(info) == ResultCode::Success, "Failed to create staging buffer!");

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = offset;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;

			vkCmdCopyBuffer(cmd, m_Handle, stagingBuffer.GetHandle(), 1, &copyRegion);

			if (cmdBuffer == VK_NULL_HANDLE)
				m_Device->EndSingleTimeCommands(cmd, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());

			(void)stagingBuffer.Map();
			(void)stagingBuffer.Flush();

			stagingBuffer.ReadFromBuffer(outData, size, 0);

			stagingBuffer.Unmap();
		}
		else // If the buffer is not device local, read directly from the buffer.
		{
			VH_ASSERT(m_Mapped, "Cannot read from unmapped buffer!");

			char* memoryOffset = reinterpret_cast<char*>(m_Mapped) + offset;

			memcpy(outData, memoryOffset, size);
		}
	}

	ResultCode Buffer::Flush(VkDeviceSize size, VkDeviceSize offset)
	{
		return (ResultCode)vmaFlushAllocation(m_Device->GetAllocator(), *m_Allocation, offset, size);;
	}

	VkDescriptorBufferInfo Buffer::DescriptorInfo()
	{
		return VkDescriptorBufferInfo
		{
			m_Handle,
			0,
			m_BufferSize,
		};
	}

	void Buffer::Move(Buffer&& other)
	{
		m_Device = other.m_Device;
		other.m_Device = nullptr;

		m_Mapped = other.m_Mapped;
		other.m_Mapped = nullptr;

		m_Handle = other.m_Handle;
		other.m_Handle = VK_NULL_HANDLE;

		m_Allocation = other.m_Allocation;
		other.m_Allocation = nullptr;

		m_BufferSize = other.m_BufferSize;
		other.m_BufferSize = 0;

		m_UsageFlags = other.m_UsageFlags;
		other.m_UsageFlags = 0;

		m_MemoryPropertyFlags = other.m_MemoryPropertyFlags;
		other.m_MemoryPropertyFlags = 0;

		m_IsDedicatedAllocation = other.m_IsDedicatedAllocation;
		other.m_IsDedicatedAllocation = false;

		other.Reset();
	}

	void Buffer::Reset()
	{
		m_Device = nullptr;
		m_Mapped = nullptr;
		m_Handle = VK_NULL_HANDLE;
		m_Allocation = nullptr;
		m_BufferSize = 0;
		m_UsageFlags = 0;
		m_MemoryPropertyFlags = 0;
		m_IsDedicatedAllocation = false;
	}

}