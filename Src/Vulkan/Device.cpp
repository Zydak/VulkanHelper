#include "Pch.h"
#include "Device.h"
#include "Logger/Logger.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "LoadedFunctions.h"

static std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

void VulkanHelper::Device::Destroy()
{
	if (m_Handle == VK_NULL_HANDLE)
		return;

	m_CommandPools.clear();

	vkDestroyDevice(m_Handle, nullptr);
	m_Handle = VK_NULL_HANDLE;
}

void VulkanHelper::Device::Init(const CreateInfo& createInfo)
{
	if (m_Handle != VK_NULL_HANDLE)
		Destroy();

	m_PhysicalDevice = createInfo.PhysicalDevice;
	VH_INFO("Selected Physical device: {0}", m_PhysicalDevice.Name);

	m_Extensions = createInfo.Extensions;
	m_Surface = createInfo.Surface;
	m_Features = createInfo.Features;
	m_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

	CreateLogicalDevice();

	CreateCommandPoolsForThread();

	CreateMemoryAllocator();
}

void VulkanHelper::Device::CreateCommandPoolsForThread()
{
	std::thread::id id = std::this_thread::get_id();
	if (!m_CommandPools.contains(id))
	{
		CommandPool::CreateInfo createInfo{};
		createInfo.Device = m_Handle;
		createInfo.Flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		createInfo.QueueFamilyIndex = Instance::Get()->FindQueueFamilies(m_PhysicalDevice.Handle, m_Surface).GraphicsFamily;
		CommandPool graphicsPool(createInfo);

		createInfo.QueueFamilyIndex = Instance::Get()->FindQueueFamilies(m_PhysicalDevice.Handle, m_Surface).ComputeFamily;
		CommandPool computePool(createInfo);

		m_CommandPools.insert({ id, std::make_unique<CommandPools>(std::move(graphicsPool), std::move(computePool)) });
	}
}

void VulkanHelper::Device::SetObjectName(VkObjectType type, uint64_t handle, const char* name) const
{
#ifndef DISTRIBUTION
	VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
	name_info.objectType = type;
	name_info.objectHandle = handle;
	name_info.pObjectName = name;

	VulkanHelper::vkSetDebugUtilsObjectNameEXT(m_Handle, &name_info);
#endif
}

VkResult VulkanHelper::Device::FindMemoryTypeIndex(uint32_t* outMemoryIndex, VkMemoryPropertyFlags flags) const
{
	VmaAllocationCreateInfo allocInfo{};
	allocInfo.priority = 0.5f;
	allocInfo.requiredFlags = flags;
	allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;

	return vmaFindMemoryTypeIndex(m_Allocator, UINT32_MAX, &allocInfo, outMemoryIndex);
}

VulkanHelper::ResultCode VulkanHelper::Device::AllocateBuffer(VkBuffer* outBuffer, VmaAllocation* outAllocation, const VkBufferCreateInfo& createInfo, VkMemoryPropertyFlags memoryPropertyFlags /*= 0*/, bool dedicatedAllocation /*= false*/)
{
	if (dedicatedAllocation)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.priority = 0.5f;
		allocCreateInfo.requiredFlags = memoryPropertyFlags;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		return (ResultCode)vmaCreateBufferWithAlignment(m_Allocator, &createInfo, &allocCreateInfo, 1, outBuffer, outAllocation, nullptr);
	}

	VH_ASSERT(false, "Not implemented!");
}

VulkanHelper::ResultCode VulkanHelper::Device::AllocateImage(VkImage* outImage, VmaAllocation* outAllocation, const VkImageCreateInfo& createInfo, VkMemoryPropertyFlags memoryPropertyFlags /*= 0*/, bool dedicatedAllocation /*= false*/)
{
	if (dedicatedAllocation)
	{
		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.priority = 0.5f;
		allocCreateInfo.requiredFlags = memoryPropertyFlags;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

		return (ResultCode)vmaCreateImage(m_Allocator, &createInfo, &allocCreateInfo, outImage, outAllocation, nullptr);
	}

	VH_ASSERT(false, "Not implemented!");
}

void VulkanHelper::Device::BeginSingleTimeCommands(VkCommandBuffer* buffer, VkCommandPool pool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = pool;
	allocInfo.commandBufferCount = 1;
	vkAllocateCommandBuffers(m_Handle, &allocInfo, buffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(*buffer, &beginInfo);
}

void VulkanHelper::Device::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool)
{
	std::mutex* queueMutex = nullptr;
	if (queue == m_GraphicsQueue)
		queueMutex = &m_GraphicsQueueMutex;
	else if (queue == m_ComputeQueue)
		queueMutex = &m_ComputeQueueMutex;

	VH_ASSERT(queueMutex != nullptr, "Queue not recognized! Upload to either Graphics or Compute queue.");

	std::unique_lock<std::mutex> queueLock(*queueMutex);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(m_Handle, pool, 1, &commandBuffer);
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

void VulkanHelper::Device::CreateMemoryAllocator()
{
	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.instance = Instance::Get()->GetHandle();
	allocatorInfo.physicalDevice = m_PhysicalDevice.Handle;
	allocatorInfo.device = m_Handle;
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorInfo.flags = VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;

	VH_CHECK(vmaCreateAllocator(&allocatorInfo, &m_Allocator) == VK_SUCCESS, "Failed to create vma allocator!");
}