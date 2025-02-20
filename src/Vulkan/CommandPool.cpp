#include "Pch.h"
#include "CommandPool.h"

#include "Logger/Logger.h"

VkResult VulkanHelper::CommandPool::Init(const CreateInfo& createInfo)
{
	if (m_Initialized)
		Destroy();

	m_Device = createInfo.Device;
	m_QueueFamilyIndex = createInfo.QueueFamilyIndex;
	m_Flags = createInfo.Flags;

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = m_QueueFamilyIndex;
	poolInfo.flags = m_Flags;

	m_Initialized = true;

	return vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_Handle);
}

void VulkanHelper::CommandPool::Destroy()
{
	if (!m_Initialized)
		return;

	vkDestroyCommandPool(m_Device, m_Handle, nullptr);

	Reset();
}

VulkanHelper::CommandPool::CommandPool(const CreateInfo& createInfo)
{
	Init(createInfo);
}

VulkanHelper::CommandPool::~CommandPool()
{
	Destroy();
}

VulkanHelper::CommandPool& VulkanHelper::CommandPool::operator=(CommandPool&& other) noexcept
{
	if (this == &other)
		return *this;

	if (m_Initialized)
		Destroy();

	Move(std::move(other));

	return *this;
}

VulkanHelper::CommandPool::CommandPool(CommandPool&& other) noexcept
{
	if (this == &other)
		return;

	if (m_Initialized)
		Destroy();

	Move(std::move(other));
}

void VulkanHelper::CommandPool::ResetCommandPool() const
{
	VH_ASSERT(m_Initialized, "CommandPool Not Initialized!");

	vkResetCommandPool(m_Device, m_Handle, 0);
}

void VulkanHelper::CommandPool::Move(CommandPool&& other) noexcept
{
	m_Handle = other.m_Handle;
	m_Device = other.m_Device;
	m_QueueFamilyIndex = other.m_QueueFamilyIndex;
	m_Flags = other.m_Flags;

	m_Initialized = other.m_Initialized;

	other.Reset();
}

void VulkanHelper::CommandPool::Reset()
{
	m_Handle = VK_NULL_HANDLE;
	m_Device = VK_NULL_HANDLE;
	m_QueueFamilyIndex = 0;
	m_Flags = 0;

	m_Initialized = false;
}
