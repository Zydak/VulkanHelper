#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "VulkanMemoryAllocator.h"
#include "Log/Log.h"

namespace VulkanHelper
{
    Expected<VulkanMemoryAllocator, VHResult> VulkanMemoryAllocator::New(const Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Memory Allocator");

        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Invalid VulkanMemoryAllocator configuration: Device is null");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.Instance == nullptr)
        {
            VH_LOG_ERROR("Invalid VulkanMemoryAllocator configuration: Instance is null");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.PhysicalDevice == nullptr)
        {
            VH_LOG_ERROR("Invalid VulkanMemoryAllocator configuration: PhysicalDevice is null");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice = config.PhysicalDevice;
        allocatorCreateInfo.device = config.Device;
        allocatorCreateInfo.instance = config.Instance;

        VmaAllocator allocator;
        VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &allocator);
        if (result != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create VMA allocator");
            return VulkanHelper::Unexpected(VHResult(result));
        }

        return VulkanMemoryAllocator(allocator);
    }

    VulkanMemoryAllocator::VulkanMemoryAllocator(VmaAllocator allocator)
        : m_Allocator(allocator)
    {
    }

    VulkanMemoryAllocator::~VulkanMemoryAllocator()
    {
        if (m_Allocator != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying Vulkan Memory Allocator");
            vmaDestroyAllocator(m_Allocator);
            m_Allocator = VK_NULL_HANDLE;
        }
    }

    VulkanMemoryAllocator::VulkanMemoryAllocator(VulkanMemoryAllocator&& other) noexcept
        : m_Allocator(other.m_Allocator)
    {
        other.m_Allocator = VK_NULL_HANDLE;
    }

    VulkanMemoryAllocator& VulkanMemoryAllocator::operator=(VulkanMemoryAllocator&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~VulkanMemoryAllocator(); // Clean up current state

        m_Allocator = other.m_Allocator;
        other.m_Allocator = VK_NULL_HANDLE;

        return *this;
    }

    VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> 
    VulkanMemoryAllocator::AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowMapping)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (allowMapping)
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        BufferAllocation allocation = {};
        VkResult result = vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &allocation.Buffer, &allocation.Allocation, nullptr);

        if (result != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to allocate buffer");
            return VulkanHelper::Unexpected(VHResult(result));
        }

        return allocation;
    }

    VulkanHelper::Expected<VulkanMemoryAllocator::ImageAllocation, VHResult> 
    VulkanMemoryAllocator::AllocateImage(const VkImageCreateInfo& imageInfo, bool allowMapping)
    {
        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        if (allowMapping)
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

        ImageAllocation allocation = {};
        VkResult result = vmaCreateImage(m_Allocator, &imageInfo, &allocInfo, &allocation.image, &allocation.Allocation, nullptr);

        if (result != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to allocate image");
            return VulkanHelper::Unexpected(VHResult(result));
        }

        return allocation;
    }

    void VulkanMemoryAllocator::DeallocateBuffer(const BufferAllocation& allocation)
    {
        vmaDestroyBuffer(m_Allocator, allocation.Buffer, allocation.Allocation);
    }

    void VulkanMemoryAllocator::DeallocateImage(const ImageAllocation& allocation)
    {
        vmaDestroyImage(m_Allocator, allocation.image, allocation.Allocation);
    }

    VulkanHelper::Expected<void*, VHResult> VulkanMemoryAllocator::MapMemory(const VmaAllocation& allocation)
    {
        void* mappedData;
        VkResult result = vmaMapMemory(m_Allocator, allocation, &mappedData);
        if (result != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to map buffer memory using VMA");
            return VulkanHelper::Unexpected(VHResult(result));
        }

        return mappedData;
    }

    void VulkanMemoryAllocator::UnmapMemory(const VmaAllocation& allocation)
    {
        vmaUnmapMemory(m_Allocator, allocation);
    }
}