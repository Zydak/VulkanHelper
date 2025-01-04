#include "pch.h"
#include "Utility/Utility.h"
#include "Core/Window.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "Device.h"
#include "Instance.h"

#include "GLFW/glfw3.h"

#include <vulkan/vulkan_core.h>
#include <vulkan/vk_platform.h>
#include <Dxgi1_2.h>

#include "vulkan.hpp"

#include "Asset/AssetManager.h"
#include "DeleteQueue.h"

namespace VulkanHelper
{
	static std::vector<const char*> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	std::vector<const char*> Device::s_DeviceExtensions;
	std::vector<Extension> Device::s_OptionalExtensions = {
		{VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME, false},
		{VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME, false}
	};

	static inline std::vector<int32_t> s_IgnoredMessageIDs;

	/*
	 * @brief Initializes vulkan instance and returns list of physical devices available on your PC.
	 */
	std::vector<Device::PhysicalDevice> Device::QueryPhysicalDevices(const PhysicalDevice::CreateInfo& createInfo)
	{
		s_UseMemoryAddressFeature = createInfo.UseMemoryAddress;

		s_DeviceExtensions = createInfo.DeviceExtensions;
		for (int i = 0; i < createInfo.OptionalExtensions.size(); i++)
		{
			s_OptionalExtensions.emplace_back(Extension{ createInfo.OptionalExtensions[i], false });
		}

		// Push VK_KHR_SWAPCHAIN_EXTENSION_NAME if it's absent
		if (std::find(s_DeviceExtensions.begin(), s_DeviceExtensions.end(), VK_KHR_SWAPCHAIN_EXTENSION_NAME) == s_DeviceExtensions.end())
			s_DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		// Perform initialization steps
		s_Features = createInfo.Features;
		s_UseRayTracing = createInfo.UseRayTracing;

		//enumerate devices
		return EnumeratePhysicalDevices(createInfo.window->GetSurface());	
	}

	/*
	 * @brief Finishes Vulkan setup with given physical device.
	 */
	void Device::Init(const PhysicalDevice& device)
	{
		if (!device.Requirements.IsSuitable())
		{
			VK_CORE_ERROR("You Chose incompatible device!");

			if (!device.Requirements.UnsupportedButRequiredExtensions.empty())
			{
				VK_CORE_ERROR("It doesn't support following extensions which are listed as required:");
				for (auto& extension : device.Requirements.UnsupportedButRequiredExtensions)
				{
					VK_CORE_ERROR("{}", extension);
				}
			}

			if (!device.Requirements.QueueIndices.IsComplete())
			{
				VK_CORE_ERROR("It doesn't support following queue families:");
				VK_CORE_ERROR("Graphics Family Queue Supported - {}", device.Requirements.QueueIndices.GraphicsFamilyHasValue);
				VK_CORE_ERROR("Present Family Queue Supported - {}", device.Requirements.QueueIndices.PresentFamilyHasValue);
				VK_CORE_ERROR("Compute Family Queue Supported - {}", device.Requirements.QueueIndices.ComputeFamilyHasValue);
			}

			if (!device.Requirements.SwapchainSupport.PresentModes.empty() && device.Requirements.SwapchainSupport.Formats.empty())
				VK_CORE_ERROR("The swapchain is incompatible!");

			VK_CORE_ASSERT(false, "");
		}

		s_PhysicalDevice = device;
		s_Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		s_Properties.pNext = &s_RayTracingProperties;
		s_RayTracingProperties.pNext = &s_AccelerationStructureProperties;
		s_AccelerationStructureProperties.pNext = &s_SubgroupProperties;
		vkGetPhysicalDeviceProperties2(s_PhysicalDevice.Handle, &s_Properties);
		s_MaxSampleCount = GetMaxSampleCount();

		VK_CORE_INFO("Selected Physical device: {0}", s_Properties.properties.deviceName);
		VK_CORE_INFO("\tSubgroup Size: {0}", s_SubgroupProperties.subgroupSize);

		// Create logical device
		CreateLogicalDevice();

		// Create command pools
		CreateCommandPools();

		// Create memory allocator
		CreateMemoryAllocator();

		// Mark the object as initialized
		s_Initialized = true;

		// Asset Manager needs device to be initialized to I do that in here
		const uint32_t coresCount = std::thread::hardware_concurrency();
		AssetManager::Init({ coresCount / 2 });

		// All objects in delete queue have to be destroyed before the device so it is managed in here
		DeleteQueue::Init({ 3 }); // I doubt there will ever be more than 3 frames in flight so that should suffice
	}

	/**
	 * @brief Destroys the Vulkan device and associated resources.
	 */
	void Device::Destroy()
	{
		vkDeviceWaitIdle(Device::GetDevice());

		DeleteQueue::Destroy();
		AssetManager::Destroy();

		// Log message indicating deletion of Vulkan Device
		VK_CORE_INFO("Deleting Vulkan Device");

		// Destroy memory pools
		for (auto& pool : s_Pools)
		{
			vmaDestroyPool(s_Allocator, pool.second);
		}
		for (auto& pool : s_ImagePools)
		{
			vmaDestroyPool(s_Allocator, pool.second);
		}
		// Destroy memory allocator
		vmaDestroyAllocator(s_Allocator);

		for (auto& pool : s_CommandPools)
		{
			// Destroy graphics command pool
			vkDestroyCommandPool(s_Device, pool.second.GraphicsCommandPool, nullptr);
			// Destroy compute command pool
			vkDestroyCommandPool(s_Device, pool.second.ComputeCommandPool, nullptr);
		}

		// Destroy Vulkan device
		vkDestroyDevice(s_Device, nullptr);

		// Mark device as uninitialized
		s_Initialized = false;
	}

	Device::~Device()
	{
		if(s_Initialized)
			Destroy();
	}

	/*
	 * @brief Checks if the required Vulkan validation layers are supported.
	 *
	 * @return True if all required validation layers are supported, false otherwise.
	 */
	bool Device::CheckValidationLayerSupport()
	{
		// Query the number of available instance layer properties
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		// Retrieve the available instance layer properties
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		// Iterate over each required validation layer
		for (const char* layerName : s_ValidationLayers) 
		{
			// Flag to track whether the current layer is found
			bool layerFound = false;

			// Iterate over available layers and check if the required layer is supported
			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) 
				{
					layerFound = true;
					break;
				}
			}

			// If the required layer is not found, return false
			if (!layerFound) { return false; }
		}

		// If all required layers are found, return true
		return true;
	}

	/*
	 * @brief Finds the Vulkan queue families supported by the physical device.
	 *
	 * @param device - Vulkan physical device for which queue families are being queried.
	 *
	 * @return QueueFamilyIndices structure containing the indices of the graphics and present queue families.
	 */
	QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		// Initialize the QueueFamilyIndices structure
		QueueFamilyIndices indices;

		// Query the number of queue families supported by the physical device
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		// Retrieve the queue family properties
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		// Iterate over each queue family to find supported capabilities
		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			// Check if the queue family supports both graphics and compute operations
			if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				indices.GraphicsFamily = i;
				indices.GraphicsFamilyHasValue = true;
			}
			// Check if the queue family supports only compute operations
			if ((queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				indices.ComputeFamily = i;
				indices.ComputeFamilyHasValue = true;
			}

			// Check if the queue family supports presentation to the associated surface
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.PresentFamily = i;
				indices.PresentFamilyHasValue = true;
			}

			// If all required queue families are found, exit the loop
			if (indices.IsComplete())
			{
				break;
			}

			// Move to the next queue family
			i++;
		}

		// Return the structure containing indices of the found queue families
		return indices;
	}

	/**
	 * @brief Creates a memory allocator for Vulkan resources.
	 */
	void Device::CreateMemoryAllocator()
	{
		// Initialize allocator creation info
		VmaAllocatorCreateInfo allocatorInfo{};
		allocatorInfo.instance = Instance::Get()->GetHandle();
		allocatorInfo.physicalDevice = s_PhysicalDevice.Handle;
		allocatorInfo.device = s_Device;
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_AMD_DEVICE_COHERENT_MEMORY_BIT;

		// Check if the memory priority extension is present
		for (VulkanHelper::Extension& ext : s_OptionalExtensions)
		{
			if (std::string(ext.Name) == std::string(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
				if (ext.supported)
					allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
		}
		for (auto ext : s_DeviceExtensions)
		{
			if (std::string(ext) == std::string(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))
				allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
		}

		// Enable buffer device address feature
		if (s_UseMemoryAddressFeature)
			allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		// Create the memory allocator
		VK_CORE_RETURN_ASSERT(vmaCreateAllocator(&allocatorInfo, &s_Allocator), VK_SUCCESS, "Failed to create VMA allocator");
	}

	/**
	 * @brief Creates a memory pool using Vulkan Memory Allocator (VMA).
	 *
	 * @param memoryIndex - Memory type index associated with the memory pool.
	 * @param pool - Reference to store the created VmaPool object.
	 * @param MBSize - Size of memory blocks in megabytes (if non-zero, takes precedence over ByteSize).
	 * @param ByteSize - Size of memory blocks in bytes (if MBSize is zero, this value is used).
	 */
	void Device::CreateMemoryPool(VkMemoryPropertyFlags propertyFlags, uint32_t memoryIndex, VmaPool& pool, VkDeviceSize MBSize, VkDeviceSize ByteSize, bool isImage)
	{
		// Initialize memory pool creation info
		VmaPoolCreateInfo poolInfo{};

		poolInfo.blockSize = 0;
		// Set block size based on MBSize or ByteSize
		if (MBSize != 0)
		{
			VK_CORE_ASSERT(ByteSize == 0, "MBSize and ByteSize can't be both set!");
			poolInfo.blockSize = 1024 * 1024 * MBSize;
		}
		else if (ByteSize != 0)
		{
			VK_CORE_ASSERT(MBSize == 0, "MBSize and ByteSize can't be both set!");
			poolInfo.blockSize = ByteSize;
		}

		// Set memory type index associated with the memory pool
		poolInfo.memoryTypeIndex = memoryIndex;

		// Set pool priority
		poolInfo.priority = 0.5f;

		// On AMD only VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT can be set as exportable and there's no need for exportable image memory
		if (isImage == false && propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{
			if (std::find(s_DeviceExtensions.begin(), s_DeviceExtensions.end(), VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) != s_DeviceExtensions.end())
			{
				// Set export memory information
				s_ExportMemoryInfo = {};
				s_ExportMemoryInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
				s_ExportMemoryInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

				poolInfo.pMemoryAllocateNext = &s_ExportMemoryInfo;
			}
		}

		// Create the memory pool
		VK_CORE_RETURN_ASSERT(vmaCreatePool(s_Allocator, &poolInfo, &pool), VK_SUCCESS, "Failed to create memory pool!");
	}

	/**
	 * @brief Searches for a memory type index that supports the specified memory property flags.
	 * It uses Vulkan Memory Allocator (VMA) to find the appropriate memory type index based on the given flags
	 * and stores the result in the provided memoryIndex reference.
	 *
	 * @param flags - Memory property flags indicating the required memory properties.
	 * @param memoryIndex - Reference to store the found memory type index.
	 */
	void Device::FindMemoryTypeIndex(VkMemoryPropertyFlags flags, uint32_t& memoryIndex)
	{
		// Initialize allocation creation info
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.priority = 0.5f;
		allocInfo.requiredFlags = flags;
		allocInfo.usage = VMA_MEMORY_USAGE_UNKNOWN;

		// Find the memory type index matching the specified flags
		VK_CORE_RETURN_ASSERT(vmaFindMemoryTypeIndex(s_Allocator, UINT32_MAX, &allocInfo, &memoryIndex), VK_SUCCESS, "Failed to find memory type index!");
	}

	/**
	 * @brief Finds the memory type index suitable for a buffer with specified creation info and memory property flags.
	 *
	 * @param createInfo - Pointer to VkBufferCreateInfo structure containing buffer creation info.
	 * @param memoryIndex - Reference to store the found memory type index.
	 * @param flags - Memory property flags indicating the required memory properties (optional).
	 */
	void Device::FindMemoryTypeIndexForBuffer(VkBufferCreateInfo& createInfo, uint32_t& memoryIndex, VkMemoryPropertyFlags flags)
	{
		// Initialize allocation creation info
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.priority = 0.5f;
		allocInfo.requiredFlags = flags;

		// Find the memory type index suitable for the buffer
		VK_CORE_RETURN_ASSERT(vmaFindMemoryTypeIndexForBufferInfo(s_Allocator, &createInfo, &allocInfo, &memoryIndex), VK_SUCCESS, "Failed to find memory type index for buffer info!");
	}

	/**
	 * @brief Finds the memory type index suitable for an image with specified creation info and memory property flags.
	 *
	 * @param createInfo - Pointer to VkImageCreateInfo structure containing image creation info.
	 * @param memoryIndex - Reference to store the found memory type index.
	 * @param flags - Memory property flags indicating the required memory properties.
	 */
	void Device::FindMemoryTypeIndexForImage(VkImageCreateInfo& createInfo, uint32_t& memoryIndex, VkMemoryPropertyFlags flags)
	{
		// Initialize allocation creation info
		VmaAllocationCreateInfo allocInfo{};
		allocInfo.priority = 0.5f;
		allocInfo.requiredFlags = flags;

		// Find the memory type index suitable for the image
		VK_CORE_RETURN_ASSERT(vmaFindMemoryTypeIndexForImageInfo(s_Allocator, &createInfo, &allocInfo, &memoryIndex), VK_SUCCESS, "Failed to find memory type index for image info!");
	}

	/**
	 * @brief Creates a Vulkan buffer along with its associated memory allocation.
	 *
	 * @param createInfo - Reference to VkBufferCreateInfo structure containing buffer creation info.
	 * @param buffer - Reference to store the created Vulkan buffer.
	 * @param alloc - Reference to store the VmaAllocation for the buffer's memory.
	 * @param customFlags - Custom memory property flags for the buffer's memory (optional).
	 * @param noPool - Flag indicating whether to create a dedicated memory pool for the buffer (optional).
	 */
	void Device::CreateBuffer(VkBufferCreateInfo& createInfo, VkBuffer& buffer, VmaAllocation& alloc, VkMemoryPropertyFlags customFlags, VmaPool* poolOut, bool noPool, VkDeviceSize minAlignment)
	{
		// Assert that the device has been initialized before creating the buffer
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");
		// Find the memory type index suitable for the buffer
		uint32_t memoryIndex = 0;
		if (std::find(s_DeviceExtensions.begin(), s_DeviceExtensions.end(), VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME) != s_DeviceExtensions.end())
		{
			s_ExternalMemoryBufferInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
			s_ExternalMemoryBufferInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;

			if (customFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
				createInfo.pNext = &s_ExternalMemoryBufferInfo;
		}

		FindMemoryTypeIndex(customFlags, memoryIndex);

		// Create buffer without using memory pool
		if (noPool)
		{
			CreateMemoryPool(customFlags, memoryIndex, *poolOut, 0, createInfo.size);

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.priority = 0.5f;
			allocCreateInfo.pool = *poolOut;

			VkResult res = vmaCreateBufferWithAlignment(s_Allocator, &createInfo, &allocCreateInfo, minAlignment, &buffer, &alloc, nullptr);
			return;
		}

		// Check if a memory pool for the memory type index already exists
		auto it = s_Pools.find(memoryIndex);
		if (it != s_Pools.end())
		{
			// Pool found for the memory type index
			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.pool = it->second;
			allocCreateInfo.priority = 0.5f;

			// Attempt to create buffer using the existing pool
			if (vmaCreateBufferWithAlignment(s_Allocator, &createInfo, &allocCreateInfo, minAlignment, &buffer, &alloc, nullptr) < 0)
			{
				// If creation fails, try again without specifying the pool
				VK_CORE_ERROR("Allocating Buffer without pool");
				allocCreateInfo.pool = nullptr;
				allocCreateInfo.requiredFlags = customFlags;
				if (vmaCreateBufferWithAlignment(s_Allocator, &createInfo, &allocCreateInfo, minAlignment, &buffer, &alloc, nullptr) < 0)
				{
					VK_CORE_ASSERT(false, "Couldn't create a buffer!");
				}
			}
		}
		else
		{
			// Pool not found for the memory type index, create a new pool
			s_Pools[memoryIndex] = VmaPool();
			CreateMemoryPool(customFlags, memoryIndex, s_Pools[memoryIndex], 300);

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.pool = s_Pools[memoryIndex];
			allocCreateInfo.priority = 0.5f;

			// Attempt to create buffer using the newly created pool
			if (vmaCreateBufferWithAlignment(s_Allocator, &createInfo, &allocCreateInfo, minAlignment, &buffer, &alloc, nullptr) < 0)
			{
				// TODO: recreate pool when it fills up instead of just not using one
				// If creation fails, try again without specifying the pool
				allocCreateInfo.pool = nullptr;
				allocCreateInfo.requiredFlags = customFlags;
				if (vmaCreateBufferWithAlignment(s_Allocator, &createInfo, &allocCreateInfo, minAlignment, &buffer, &alloc, nullptr) < 0)
				{
					VK_CORE_ASSERT(false, "Couldn't create a buffer!");
				}
			}
		}
	}

	/**
	 * @brief Creates a Vulkan image along with its associated memory allocation.
	 *
	 * @param createInfo - Reference to VkImageCreateInfo structure containing image creation info.
	 * @param image - Reference to store the created Vulkan image.
	 * @param alloc - Reference to store the VmaAllocation for the image's memory.
	 * @param customFlags - Custom memory property flags for the image's memory (optional).
	 */
	void Device::CreateImage(VkImageCreateInfo& createInfo, VkImage& image, VmaAllocation& alloc, VkMemoryPropertyFlags customFlags)
	{
		// Assert that the device has been initialized before creating the image
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// Find the memory type index suitable for the image
		uint32_t memoryIndex = 0;
		FindMemoryTypeIndex(customFlags, memoryIndex);

		auto it = s_ImagePools.find(memoryIndex);
		if (it != s_ImagePools.end())
		{
			// Pool found for the memory type index
			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.pool = it->second;
			allocCreateInfo.memoryTypeBits = memoryIndex;

			// Attempt to create image using the existing pool
			if (vmaCreateImage(s_Allocator, &createInfo, &allocCreateInfo, &image, &alloc, nullptr) < 0)
			{
				// If creation fails, try again without specifying the pool
				allocCreateInfo.pool = nullptr;
				allocCreateInfo.memoryTypeBits = memoryIndex;
				VK_CORE_ERROR("Allocating Image without pool");

				if (vmaCreateImage(s_Allocator, &createInfo, &allocCreateInfo, &image, &alloc, nullptr) < 0)
				{
					VK_CORE_ASSERT(false, "Couldn't create an image!");
				}
				static int x = 0;
				VK_CORE_TRACE("Allocation {}", x);
				x++;
			}
		}
		else
		{
			// Pool not found for the memory type index, create a new pool
			s_ImagePools[memoryIndex] = VmaPool();
			CreateMemoryPool(customFlags, memoryIndex, s_ImagePools[memoryIndex], 300, 0, true);

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.pool = s_ImagePools[memoryIndex];

			if (vmaCreateImage(s_Allocator, &createInfo, &allocCreateInfo, &image, &alloc, nullptr) < 0)
			{
				VK_CORE_ASSERT(false, "Couldn't create an image!");
			}
		}

		// // Assert that the device has been initialized before creating the image
		// VL_CORE_ASSERT(s_Initialized, "Device not Initialized!");
		// 
		// // Find the memory type index suitable for the image
		// uint32_t memoryIndex = 0;
		// FindMemoryTypeIndexForImage(createInfo, memoryIndex, customFlags);
		// 
		// // Initialize allocation creation info
		// VmaAllocationCreateInfo allocCreateInfo = {};
		// allocCreateInfo.memoryTypeBits = memoryIndex;
		// 
		// // Create the image and allocate memory for it
		// VL_CORE_RETURN_ASSERT(
		// 	vmaCreateImage(s_Allocator, &createInfo, &allocCreateInfo, &image, &alloc, nullptr),
		// 	VK_SUCCESS,
		// 	"Couldn't create an image!"
		// );
	}

	/**
	 * @brief Sets a debug name for a Vulkan object, allowing for easier identification
	 * and debugging of objects in Vulkan applications. It is typically used for objects like
	 * buffers, images, etc., to provide meaningful names for better debugging.
	 *
	 * @param type - Type of the Vulkan object (e.g., VK_OBJECT_TYPE_BUFFER).
	 * @param handle - Handle of the Vulkan object.
	 * @param name - Pointer to a null-terminated string specifying the debug name.
	 */
	void Device::SetObjectName(VkObjectType type, uint64_t handle, const char* name)
	{
#ifndef DISTRIBUTION

		// Assert that the device has been initialized before setting the object name
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// Initialize object name info structure
		VkDebugUtilsObjectNameInfoEXT name_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		name_info.objectType = type;
		name_info.objectHandle = handle;
		name_info.pObjectName = name;

		// Set the debug name for the Vulkan object
		Device::vkSetDebugUtilsObjectNameEXT(Device::GetDevice(), &name_info);

#endif
	}

	/**
	 * @brief Begins a debug label for a command buffer, allowing for easier identification
	 * and debugging of command buffer operations. It is typically used to mark the start of a section
	 * of command buffer operations with a meaningful label and color.
	 *
	 * @param cmd - Command buffer to which the debug label applies.
	 * @param name - Pointer to a null-terminated string specifying the debug label name.
	 * @param color - Color associated with the debug label.
	 */
	void Device::BeginLabel(VkCommandBuffer cmd, const char* name, glm::vec4 color)
	{
#ifndef DISTRIBUTION

		// Assert that the device has been initialized before beginning the label
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// Initialize debug label info structure
		VkDebugUtilsLabelEXT utilsInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, name, *(float*)&color };

		// Begin the debug label for the command buffer
		Device::vkCmdBeginDebugUtilsLabelEXT(cmd, &utilsInfo);

#endif
	}
	
	/**
	 * @brief Ends the current debug label for a command buffer.
	 * 
	 * @param cmd - Command buffer to which the debug label applies.
	 */
	void Device::EndLabel(VkCommandBuffer cmd)
	{
		// Check if debug or release mode is enabled (not in distribution mode)
#ifndef DISTRIBUTION

		// Assert that the device has been initialized before ending the label
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// End the debug label for the command buffer
		Device::vkCmdEndDebugUtilsLabelEXT(cmd);

#endif
	}

	/**
	 * @brief Inserts a debug label into a command buffer at the current point, allowing for
	 * additional labeling of specific points in command buffer operations. It is typically used to mark
	 * specific points of interest or sections within a command buffer with meaningful labels and colors.
	 *
	 * @param cmd - Command buffer to which the debug label applies.
	 * @param name - Pointer to a null-terminated string specifying the debug label name.
	 * @param color - Color associated with the debug label.
	 */
	void Device::InsertLabel(VkCommandBuffer cmd, const char* name, glm::vec4 color)
	{
		// Check if debug or release mode is enabled (not in distribution mode)
#ifndef DISTRIBUTION

		// Assert that the device has been initialized before inserting the label
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// Initialize debug label info structure
		VkDebugUtilsLabelEXT utilsInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, nullptr, name, *(float*)&color };

		// Insert the debug label into the command buffer at the current point
		Device::vkCmdInsertDebugUtilsLabelEXT(cmd, &utilsInfo);

#endif
	}

	void Device::CreateCommandPoolForThread()
	{
		CreateCommandPools();
	}

	Device::PhysicalDeviceRequirements Device::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		Device::PhysicalDeviceRequirements requirements;

		// Find queue families supported by the device
		requirements.QueueIndices = FindQueueFamilies(device, surface);

		// Check if required device extensions are supported by the device
		requirements.UnsupportedButRequiredExtensions = CheckDeviceExtensionSupport(device);

		// Check if the swap chain is adequate (availability of formats and present modes)
		requirements.SwapchainSupport = QuerySwapchainSupport(device, surface);
		//swapChainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();

		return requirements;
	}

	std::vector<Device::PhysicalDevice> Device::EnumeratePhysicalDevices(VkSurfaceKHR surface)
	{
		// Enumerate all physical devices available in the Vulkan instance
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(Instance::Get()->GetHandle(), &deviceCount, nullptr);
		VK_CORE_ASSERT(deviceCount != 0, "failed to find GPUs with Vulkan support!");
		VK_CORE_INFO("Number of devices: {0}", deviceCount);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(Instance::Get()->GetHandle(), &deviceCount, devices.data());

		std::vector<Device::PhysicalDevice> outDevices(deviceCount);

		// Iterate through all physical devices to find the most suitable one
		for (uint32_t i = 0; i < deviceCount; i++) 
		{
			outDevices[i].Handle = devices[i];
			outDevices[i].Requirements = IsDeviceSuitable(devices[i], surface);
			{
				// Retrieve and store properties of the selected physical device for later use
				VkPhysicalDeviceProperties2 properties2{};
				properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				vkGetPhysicalDeviceProperties2(devices[i], &properties2);

				// Determine the vendor
				switch (properties2.properties.vendorID)
				{
				case 0x1002:
					outDevices[i].Vendor = Vendor::AMD;
					break;

				case 0x10DE:
					outDevices[i].Vendor = Vendor::NVIDIA;
					break;

				case 0x8086:
					outDevices[i].Vendor = Vendor::INTEL;
					break;

				case 0x1010:
					outDevices[i].Vendor = Vendor::ImgTec;
					break;

				case 0x13B5:
					outDevices[i].Vendor = Vendor::ARM;
					break;

				case 0x5143:
					outDevices[i].Vendor = Vendor::Qualcomm;
					break;

				default:
					outDevices[i].Vendor = Vendor::Unknown;
					break;
				}

				outDevices[i].Name = properties2.properties.deviceName;

				// Prioritize discrete GPUs over integrated GPUs if available
				if (properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					outDevices[i].Discrete = true;
				}
			}
		}

		return outDevices;
	}

	/**
	 * @brief Creates a logical device which acts as an interface to interact with the physical device.
	 * It configures device queues, enables required features, and sets up necessary extensions.
	 */
	void Device::CreateLogicalDevice()
	{
		// Find queue families supported by the physical device
		QueueFamilyIndices indices = s_PhysicalDevice.Requirements.QueueIndices;

		// Prepare queue creation information
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };

		std::vector<float> queuePriorities = { 1.0f, 1.0f };
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo = {};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = queuePriorities.data();
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// Prepare extensions for the device
		std::vector<const char*> extensions;
		for (auto& extension : s_DeviceExtensions)
			extensions.push_back(extension);

		for (auto& extension : s_OptionalExtensions)
			if (extension.supported)
				extensions.push_back(extension.Name);

		// Create device creation information
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = nullptr;
		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
		s_Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		createInfo.pNext = &s_Features;

		// Enable validation layers if required
		if (s_EnableValidationLayers)
		{
			createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
			createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
		}

		// Create the logical device
		VK_CORE_RETURN_ASSERT(vkCreateDevice(s_PhysicalDevice.Handle, &createInfo, nullptr, &s_Device),
			VK_SUCCESS,
			"failed to create logical device!"
		);

		// Retrieve device queues
		vkGetDeviceQueue(s_Device, indices.GraphicsFamily, 0, &s_GraphicsQueue);
		vkGetDeviceQueue(s_Device, indices.PresentFamily, 0, &s_PresentQueue);
		vkGetDeviceQueue(s_Device, indices.ComputeFamily, 0, &s_ComputeQueue);
	}

	std::set<std::string> Device::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		// Get the count of available device extensions
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		// Retrieve the available device extensions
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// Convert the set of required extensions to a set of strings
		std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());

		// Iterate through available extensions and remove them from the set of required extensions if found
		for (const auto& extension : availableExtensions) 
		{
			requiredExtensions.erase(extension.extensionName);
		}

		// Iterate through available extensions again to mark optional extensions as supported if available
		for (const auto& extension : availableExtensions)
		{
			Extension ext;
			ext.Name = extension.extensionName;
			for (auto& optionalExtension : s_OptionalExtensions)
			{
				if (std::string(optionalExtension.Name) == std::string(ext.Name))
				{
					optionalExtension.supported = true;
				}
			}
		}

		// Return true if all required extensions are supported, false otherwise
		return requiredExtensions;
	}

	/**
	 * @brief Retrieves various details about the swapchain support for the given physical device,
	 * including surface capabilities, supported surface formats, and presentation modes.
	 *
	 * @param device - Vulkan physical device to query swapchain support for.
	 * @return A structure containing swapchain support details.
	 */
	SwapchainSupportDetails Device::QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
	{
		// Initialize the details structure
		SwapchainSupportDetails details;

		// Retrieve surface capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

		// Retrieve supported surface formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.Formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
		}

		// Retrieve supported presentation modes
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			details.PresentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
		}

		// Return the swapchain support details
		return details;
	}

	/**
	 * @brief Retrieves the maximum supported sample count for framebuffers.
	 * 
	 * @return The maximum sample count supported.
	 */
	VkSampleCountFlagBits Device::GetMaxSampleCount()
	{
		// Retrieve the supported sample counts for both color and depth framebuffers
		VkSampleCountFlags counts = s_Properties.properties.limits.framebufferColorSampleCounts & s_Properties.properties.limits.framebufferDepthSampleCounts;

		// Check for the highest supported sample count
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		// If no higher sample count is supported, return the default of 1 sample per pixel
		return VK_SAMPLE_COUNT_1_BIT;
	}

	/**
	 * @brief Finds a supported format among the given candidates.
	 *
	 * @param candidates - List of candidate formats to check.
	 * @param tiling - Desired tiling mode for the format.
	 * @param features - Desired format features.
	 * 
	 * @return The first supported format found among the candidates, or VK_FORMAT_UNDEFINED if none are supported.
	 */
	VkFormat Device::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		// Ensure that the device is initialized
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// Iterate through each candidate format
		for (VkFormat format : candidates)
		{
			// Retrieve format properties
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(s_PhysicalDevice.Handle, format, &props);

			// Check if the format supports the specified tiling and features
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) { return format; }
		}

		// Return VK_FORMAT_UNDEFINED if no supported format is found among the candidates
		return VK_FORMAT_UNDEFINED;
	}

	/**
	 * @brief Creates command pools for graphics and compute queues.
	 */
	void Device::CreateCommandPools()
	{
		// Find queue family indices for graphics and compute queues
		QueueFamilyIndices queueFamilyIndices = s_PhysicalDevice.Requirements.QueueIndices;

		s_CommandPools[std::this_thread::get_id()] = CommandPool{};

		// Create command pool for graphics queue
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_CORE_RETURN_ASSERT(vkCreateCommandPool(s_Device, &poolInfo, nullptr, &GetGraphicsCommandPool()),
				VK_SUCCESS,
				"failed to create graphics command pool!"
			);
		}

		// Create command pool for compute queue
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamilyIndices.ComputeFamily;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VK_CORE_RETURN_ASSERT(vkCreateCommandPool(s_Device, &poolInfo, nullptr, &GetComputeCommandPool()),
				VK_SUCCESS,
				"failed to create compute command pool!"
			);
		}
	}

	/**
	 * @brief Searches for a memory type that matches the specified type filter and memory properties.
	 *
	 * @param typeFilter - Bit field indicating the memory types that are suitable.
	 * @param properties - Desired memory properties.
	 * @return - Index of a suitable memory type.
	 */
	uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		// Ensure that the device is initialized
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		/*
			The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
			Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM
			for when VRAM runs out. The different types of memory exist within these heaps.
			Right now we'll only concern ourselves with the type of memory and not the heap it comes
			from, but you can imagine that this can affect performance.
		*/
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(s_PhysicalDevice.Handle, &memProperties);

		// Iterate through each memory type
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			/*
				The typeFilter parameter will be used to specify the bit field of memory
				types that are suitable. That means that we can find the index of a suitable
				memory type by simply iterating over them and checking if the corresponding bit is set to 1.
			*/
			/*
				We also need to be able to write our vertex data to that memory.
				The memoryTypes array consists of VkMemoryType structs that specify
				the heap and properties of each type of memory. The properties define
				special features of the memory, like being able to map it so we can write to it from the CPU.
				So we should check if the result of the bitwise AND is equal to the desired properties bit field.
			*/
			if ((typeFilter & (1 << i)) && ((memProperties.memoryTypes[i].propertyFlags & properties) == properties)) 
			{
				// Return the index of the suitable memory type
				return i;
			}
		}

		// Assert if no suitable memory type is found
		VK_CORE_ASSERT(false, "failed to find suitable memory type!");
		return 0;
	}

	/**
	 * @brief Allocates and begins recording commands into a single-time use command buffer.
	 * Single-time command buffers are used for short-lived operations that need to be submitted to the device only once.
	 *
	 * @param buffer - Reference to the allocated command buffer.
	 * @param pool - command pool from which the command buffer is allocated.
	 */
	void Device::BeginSingleTimeCommands(VkCommandBuffer& buffer, VkCommandPool pool)
	{
		// Ensure that the device is initialized
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		// Allocate a command buffer from the specified pool
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = pool;
		allocInfo.commandBufferCount = 1;
		vkAllocateCommandBuffers(s_Device, &allocInfo, &buffer);

		// Begin recording commands into the allocated command buffer
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(buffer, &beginInfo);
	}

	/**
	 * @brief Ends the recording of commands in the specified command buffer, submits the command buffer
	 * to the specified queue for execution, waits for the queue to become idle, and then frees the command buffer.
	 * Single-time command buffers are typically used for short-lived operations that need to be submitted to the device only once.
	 *
	 * @param commandBuffer - Command buffer to be ended, submitted, and freed.
	 * @param queue - Vulkan queue where the command buffer will be submitted for execution.
	 * @param pool - Command pool from which the command buffer was allocated.
	 */
	void Device::EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool)
	{
		// Ensure that the device is initialized
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		std::mutex* queueMutex = nullptr;
		if (queue == s_GraphicsQueue)
			queueMutex = &s_GraphicsQueueMutex;
		else if (queue == s_ComputeQueue)
			queueMutex = &s_ComputeQueueMutex;

		VK_CORE_ASSERT(queueMutex != nullptr, "?????");

		std::unique_lock<std::mutex> queueLock(*queueMutex);

		// End recording of commands in the command buffer
		vkEndCommandBuffer(commandBuffer);

		// Submit the command buffer for execution to the specified queue
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

		// Wait for the submitted queue to become idle
		vkQueueWaitIdle(queue);

		// Free the command buffer from the specified pool
		vkFreeCommandBuffers(s_Device, pool, 1, &commandBuffer);
	}

	// ---------------------------------
	// Variable Definitions
	// ---------------------------------

	VmaAllocator Device::s_Allocator;
	std::unordered_map<uint32_t, VmaPool> Device::s_Pools;
	VkExportMemoryAllocateInfo Device::s_ExportMemoryInfo;
	VkExternalMemoryBufferCreateInfo Device::s_ExternalMemoryBufferInfo;
	VkExternalMemoryImageCreateInfo Device::s_ExternalMemoryImageInfo;
	VkPhysicalDeviceProperties2 Device::s_Properties = {};
	VkSampleCountFlagBits Device::s_MaxSampleCount;
	VkPhysicalDeviceFeatures2 Device::s_Features;
	VkDebugUtilsMessengerEXT Device::s_DebugMessenger = {};
	VkDevice Device::s_Device = {};
	bool Device::s_UseMemoryAddressFeature;
	VkQueue Device::s_GraphicsQueue = {};
	std::mutex Device::s_GraphicsQueueMutex;
	VkQueue Device::s_PresentQueue = {};
	std::unordered_map<std::thread::id, VulkanHelper::CommandPool> Device::s_CommandPools;
	VkQueue Device::s_ComputeQueue = {};
	std::mutex Device::s_ComputeQueueMutex;
	bool Device::s_UseRayTracing;
	bool Device::s_Initialized = false;

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR Device::s_RayTracingProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
	VkPhysicalDeviceAccelerationStructurePropertiesKHR Device::s_AccelerationStructureProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };
	VkPhysicalDeviceSubgroupProperties Device::s_SubgroupProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES };

	// -----------------------------------
	// Loaded functions
	// -----------------------------------

	VkResult Device::vkCreateAccelerationStructureKHR(VkDevice device, VkAccelerationStructureCreateInfoKHR* createInfo, VkAccelerationStructureKHR* structure)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCreateAccelerationStructureKHR");
		if (func != nullptr) { return func(device, createInfo, nullptr, structure); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	void Device::vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR structure)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkDestroyAccelerationStructureKHR");
		if (func != nullptr) { return func(device, structure, nullptr); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdBuildAccelerationStructuresKHR");
		if (func != nullptr) { return func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdWriteAccelerationStructuresPropertiesKHR");
		if (func != nullptr) { return func(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR* pInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdCopyAccelerationStructureKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdCopyAccelerationStructureKHR");
		if (func != nullptr) { return func(commandBuffer, pInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkGetAccelerationStructureBuildSizesKHR");
		if (func != nullptr) { return func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	VkResult Device::vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCreateRayTracingPipelinesKHR");
		if (func != nullptr) { return func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	VkDeviceAddress Device::vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkGetAccelerationStructureDeviceAddressKHR");
		if (func != nullptr) { return func(device, pInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	VkResult Device::vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkGetRayTracingShaderGroupHandlesKHR");
		if (func != nullptr) { return func(device, pipeline, firstGroup, groupCount, dataSize, pData); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	void Device::vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdTraceRaysKHR");
		if (func != nullptr) { return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdPushDescriptorSetKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdPushDescriptorSetKHR");
		if (func != nullptr) { return func(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	VkResult Device::vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkGetMemoryWin32HandleKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkGetMemoryWin32HandleKHR");
		if (func != nullptr) { return func(device, pGetWin32HandleInfo, pHandle); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	VkResult Device::vkGetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkGetSemaphoreWin32HandleKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkGetSemaphoreWin32HandleKHR");
		if (func != nullptr) { return func(device, pGetWin32HandleInfo, pHandle); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	VkResult Device::vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkSetDebugUtilsObjectNameEXT");
		if (func != nullptr) { return func(device, pNameInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
	}

	void Device::vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdInsertDebugUtilsLabelEXT");
		if (func != nullptr) { return func(commandBuffer, pLabelInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdEndDebugUtilsLabelEXT");
		if (func != nullptr) { return func(commandBuffer); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdBeginDebugUtilsLabelEXT");
		if (func != nullptr) { return func(commandBuffer, pLabelInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdBeginRenderingKHR(VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdBeginRenderingKHR");
		if (func != nullptr) { return func(commandBuffer, pRenderingInfo); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

	void Device::vkCmdEndRenderingKHR(VkCommandBuffer commandBuffer)
	{
		VK_CORE_ASSERT(s_Initialized, "Device not Initialized!");

		static auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(Instance::Get()->GetHandle(), "vkCmdEndRenderingKHR");
		if (func != nullptr) { return func(commandBuffer); }
		else { VK_CORE_ASSERT(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); }
	}

}