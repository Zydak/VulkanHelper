#include "Pch.h"

#include "Buffer.h"
#include "Logger/Logger.h"

namespace VulkanHelper
{
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

		m_Initialized = false;
	}

	VkResult Buffer::Init(const Buffer::CreateInfo& createInfo)
	{
		if (m_Initialized)
			Destroy();
		
		m_Initialized = true;

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

		return m_Device->CreateBuffer(&m_Handle, m_Allocation, bufferInfo, m_MemoryPropertyFlags, createInfo.DedicatedAllocation);
	}

	void Buffer::Destroy()
	{
		if (!m_Initialized)
			return;

		Unmap();

		//DeleteQueue::TrashBuffer(*this);

		delete m_Allocation;
		vkDestroyBuffer(m_Device->GetHandle(), m_Handle, nullptr);

		Reset();
	}

	Buffer::Buffer(const Buffer::CreateInfo& createInfo)
	{
		Init(createInfo);
	}

	Buffer::Buffer(Buffer&& other) noexcept
	{
		if (this == &other)
			return;

		if (m_Initialized)
			Destroy();

		Move(std::move(other));
	}

	Buffer& Buffer::operator=(Buffer&& other) noexcept
	{
		if (this == &other)
			return *this;

		if (m_Initialized)
			Destroy();

		Move(std::move(other));

		return *this;
	}

	Buffer::~Buffer()
	{
		Destroy();
	}

	VmaAllocationInfo Buffer::GetMemoryInfo() const
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");

		VmaAllocationInfo info{};
		vmaGetAllocationInfo(m_Device->GetAllocator(), *m_Allocation, &info);

		return info;
	}

	VkDeviceAddress Buffer::GetDeviceAddress() const
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");

		VkBufferDeviceAddressInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		info.buffer = m_Handle;

		return vkGetBufferDeviceAddress(m_Device->GetHandle(), &info);
	}

	VkResult Buffer::Map(VkDeviceSize size, VkDeviceSize offset)
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");
		VH_ASSERT(!(m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), "Can't map device local buffer!");

		VkResult result = vmaMapMemory(m_Device->GetAllocator(), *m_Allocation, &m_Mapped);

		return result;
	}

	void Buffer::Unmap()
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");

		if (m_Mapped)
		{
			vmaUnmapMemory(m_Device->GetAllocator(), *m_Allocation);
			m_Mapped = nullptr;
		}
	}

	void Buffer::Barrier(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkCommandBuffer cmd)
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");

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

	void Buffer::WriteToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset, VkCommandBuffer cmdBuffer)
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");
		VH_ASSERT((size == VK_WHOLE_SIZE || size + offset <= m_BufferSize), "Data size is larger than buffer size, either resize the buffer or create a larger one");
		VH_ASSERT(data != nullptr, "Invalid data pointer");

		if (size == VK_WHOLE_SIZE)
			size = m_BufferSize - offset;

		VkCommandBuffer cmd;
		if (cmdBuffer == VK_NULL_HANDLE)
			m_Device->BeginSingleTimeCommands(&cmd, m_Device->GetGraphicsCommandPool()->GetHandle());
		else
			cmd = cmdBuffer;

		// If the buffer is device local, use a staging buffer to transfer the data.
		if (m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			Buffer::CreateInfo info{};
			info.BufferSize = size;
			info.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			info.UsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			info.Device = m_Device;
			Buffer stagingBuffer(info);

			stagingBuffer.Map();

			stagingBuffer.WriteToBuffer(data, size, 0, cmd);
			stagingBuffer.Flush();

			stagingBuffer.Unmap();

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = 0;
			copyRegion.dstOffset = offset;
			copyRegion.size = size;

			vkCmdCopyBuffer(cmd, stagingBuffer.GetHandle(), m_Handle, 1, &copyRegion);
		}
		else // If the buffer is not device local, write directly to the buffer.
		{
			VH_ASSERT(m_Mapped, "Cannot copy to unmapped buffer");

			char* memOffset = reinterpret_cast<char*>(m_Mapped) + offset;

			memcpy(memOffset, data, size);
		}

		if (cmdBuffer == VK_NULL_HANDLE)
			m_Device->EndSingleTimeCommands(cmd, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());
	}

	void Buffer::ReadFromBuffer(void* outData, VkDeviceSize size /*= VK_WHOLE_SIZE*/, VkDeviceSize offset /*= 0*/)
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");
		VH_ASSERT((size == VK_WHOLE_SIZE || size + offset <= m_BufferSize), "Data size is larger than buffer size on reading!");
		VH_ASSERT(outData != nullptr, "Invalid outData pointer");

		if (size == VK_WHOLE_SIZE)
			size = m_BufferSize - offset;

		if (m_MemoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			VH_ASSERT((m_UsageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) != 0, "Can't read from buffer that is DEVICE_LOCAL and wasn't create with USAGE_TRANSFERS_SRC_BIT!");

			Buffer::CreateInfo info{};
			info.BufferSize = size;
			info.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			info.UsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			info.Device = m_Device;
			Buffer stagingBuffer(info);

			VkCommandBuffer cmd;
			m_Device->BeginSingleTimeCommands(&cmd, m_Device->GetGraphicsCommandPool()->GetHandle());

			VkBufferCopy copyRegion{};
			copyRegion.srcOffset = offset;
			copyRegion.dstOffset = 0;
			copyRegion.size = size;

			vkCmdCopyBuffer(cmd, m_Handle, stagingBuffer.GetHandle(), 1, &copyRegion);

			m_Device->EndSingleTimeCommands(cmd, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle()); // End the command to synch with CPU

			stagingBuffer.Map();
			stagingBuffer.Flush();

			stagingBuffer.ReadFromBuffer(outData, size, 0);

			stagingBuffer.Unmap();
		}
		else // If the buffer is not device local, read directly to the buffer.
		{
			VH_ASSERT(m_Mapped, "Cannot read from unmapped buffer!");

			char* memoryOffset = reinterpret_cast<char*>(m_Mapped) + offset;

			memcpy(outData, memoryOffset, size);
		}
	}

	VkResult Buffer::Flush(VkDeviceSize size, VkDeviceSize offset)
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");

		return vmaFlushAllocation(m_Device->GetAllocator(), *m_Allocation, offset, size);;
	}

	VkDescriptorBufferInfo Buffer::DescriptorInfo()
	{
		VH_ASSERT(m_Initialized, "Buffer Not Initialized!");

		return VkDescriptorBufferInfo
		{
			m_Handle,
			0,
			m_BufferSize,
		};
	}

	void Buffer::Move(Buffer&& other)
	{
		m_Device = std::move(other.m_Device);
		m_Mapped = std::move(other.m_Mapped);
		m_Handle = std::move(other.m_Handle);
		m_Allocation = std::move(other.m_Allocation);

		m_BufferSize = std::move(other.m_BufferSize);
		m_UsageFlags = std::move(other.m_UsageFlags);
		m_MemoryPropertyFlags = std::move(other.m_MemoryPropertyFlags);
		m_IsDedicatedAllocation = std::move(other.m_IsDedicatedAllocation);

		m_Initialized = other.m_Initialized;

		other.Reset();
	}
}