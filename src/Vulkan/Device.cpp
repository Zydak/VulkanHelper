#include "Pch.h"
#include "Device.h"
#include "Logger/Logger.h"

static std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

void VulkanHelper::Device::Init(const CreateInfo& createInfo)
{
	if (m_Initialized)
		Destroy();

	m_PhysicalDevice = createInfo.PhysicalDevice;
	VH_INFO("Selected Physical device: {0}", m_PhysicalDevice.Name);

	m_Extensions = createInfo.Extensions;
	m_Surface = createInfo.Surface;
	m_Features = createInfo.Features;
	m_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	CreateLogicalDevice();

	CreateCommandPoolsForThread();

	m_Initialized = true;
}

void VulkanHelper::Device::Destroy()
{
	if (!m_Initialized)
		return;

	Reset();
}

VulkanHelper::Device::Device(const CreateInfo& createInfo)
{
	Init(createInfo);
}

void VulkanHelper::Device::CreateCommandPoolsForThread()
{
	std::thread::id id = std::this_thread::get_id();
	if (m_CommandPools.find(id) == m_CommandPools.end())
	{
		CommandPools pools;

		CommandPool::CreateInfo createInfo{};
		createInfo.Device = m_Handle;
		createInfo.Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		createInfo.QueueFamilyIndex = Instance::Get()->FindQueueFamilies(m_PhysicalDevice.Handle, m_Surface).GraphicsFamily;
		pools.Graphics.Init(createInfo);

		createInfo.QueueFamilyIndex = Instance::Get()->FindQueueFamilies(m_PhysicalDevice.Handle, m_Surface).ComputeFamily;
		pools.Compute.Init(createInfo);

		m_CommandPools[id] = std::move(pools);
	}
}

VulkanHelper::Device::~Device()
{
	Destroy();
}

void VulkanHelper::Device::CreateLogicalDevice()
{
	Instance::PhysicalDevice::QueueFamilyIndices indices = Instance::Get()->FindQueueFamilies(m_PhysicalDevice.Handle, m_Surface);

	std::vector<uint32_t> families = { indices.GraphicsFamily, indices.ComputeFamily };
	if (indices.NeedsPresentFamily)
	{
		families.push_back(indices.PresentFamily);
	}
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	float queuePriority = 1.0f;
	// Upload only unique queue families, sometimes the same family can be used for multiple operations (graphics queue with presentation capabilities is very common)
	for (size_t i = 0; i < families.size(); i++)
	{
		bool alreadyPresent = false;
		for (size_t j = 0; j < queueCreateInfos.size(); j++)
		{
			if (queueCreateInfos[j].queueFamilyIndex == families[i])
			{
				alreadyPresent = true;
				break;
			}
		}

		if (!alreadyPresent)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueCreateInfo.queueFamilyIndex = families[i];
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}
	}

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = nullptr;
	createInfo.enabledExtensionCount = (uint32_t)m_Extensions.size();
	createInfo.ppEnabledExtensionNames = m_Extensions.data();
	createInfo.pNext = &m_Features;

#ifndef DISTRIBUTION
	createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
	createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
#endif

	VH_CHECK(vkCreateDevice(m_PhysicalDevice.Handle, &createInfo, nullptr, &m_Handle) == VK_SUCCESS, "Failed to create logical device!");

	vkGetDeviceQueue(m_Handle, indices.GraphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_Handle, indices.ComputeFamily, 0, &m_ComputeQueue);

	if (indices.NeedsPresentFamily)
		vkGetDeviceQueue(m_Handle, indices.PresentFamily, 0, &m_PresentQueue);
}

void VulkanHelper::Device::Move(Device&& other) noexcept
{

}

void VulkanHelper::Device::Reset()
{

}

VulkanHelper::Device& VulkanHelper::Device::operator=(Device&& other) noexcept
{
	return *this;
}

VulkanHelper::Device::Device(Device&& other) noexcept
{

}
