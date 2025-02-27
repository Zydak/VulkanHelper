#include "Pch.h"
#include "CommandPool.h"

#include "Logger/Logger.h"

VulkanHelper::CommandPool::CommandPool(const CreateInfo& createInfo)
{
	m_Device = createInfo.Device;
	m_QueueFamilyIndex = createInfo.QueueFamilyIndex;
	m_Flags = createInfo.Flags;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_QueueFamilyIndex;
	poolInfo.flags = m_Flags;

	vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_Handle);
}

VulkanHelper::CommandPool::~CommandPool()
{
	Destroy();
}

void VulkanHelper::CommandPool::Destroy()
{
	if (m_Handle != VK_NULL_HANDLE)
		vkDestroyCommandPool(m_Device, m_Handle, nullptr);

	m_Handle = VK_NULL_HANDLE;
}

VulkanHelper::CommandPool& VulkanHelper::CommandPool::operator=(CommandPool&& other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	Move(std::move(other));

	return *this;
}

VulkanHelper::CommandPool::CommandPool(CommandPool&& other) noexcept
{
	if (this == &other)
		return;

	Destroy();

	Move(std::move(other));
}

void VulkanHelper::CommandPool::ResetCommandPool() const
{
	vkResetCommandPool(m_Device, m_Handle, 0);
}

void VulkanHelper::CommandPool::Move(CommandPool&& other) noexcept
{
	m_Handle = other.m_Handle;
	other.m_Handle = VK_NULL_HANDLE;

	m_Device = other.m_Device;
	other.m_Device = VK_NULL_HANDLE;

	m_QueueFamilyIndex = other.m_QueueFamilyIndex;
	other.m_QueueFamilyIndex = 0;

	m_Flags = other.m_Flags;
	other.m_Flags = 0;
}
