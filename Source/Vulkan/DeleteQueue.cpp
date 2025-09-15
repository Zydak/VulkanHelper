#include "DeleteQueue.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    DeleteQueue::DeleteQueue(VkDevice device, VulkanMemoryAllocator* memoryAllocator, uint32_t framesDelay)
        : m_Device(device), m_FramesDelay(framesDelay), m_MemoryAllocator(memoryAllocator)
    {
        VH_LOG_INFO("DeleteQueue created with {} frames delay", m_FramesDelay);
    }

    DeleteQueue::~DeleteQueue()
    {
        if (!m_DeletionQueue.Empty())
        {
            VH_LOG_WARN("DeleteQueue destroyed with {} pending deletions, flushing remaining objects", m_DeletionQueue.Size());
            Flush();
        }
        VH_LOG_INFO("DeleteQueue destroyed");
    }

    DeleteQueue::DeleteQueue(DeleteQueue&& other) noexcept
        : m_Device(other.m_Device)
        , m_FramesDelay(other.m_FramesDelay)
        , m_DeletionQueue(std::move(other.m_DeletionQueue))
        , m_MemoryAllocator(other.m_MemoryAllocator)
    {
        other.m_Device = VK_NULL_HANDLE;
        other.m_FramesDelay = 0;
    }

    DeleteQueue& DeleteQueue::operator=(DeleteQueue&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (!m_DeletionQueue.Empty())
        {
            VH_LOG_WARN("DeleteQueue destroyed with {} pending deletions, flushing remaining objects", m_DeletionQueue.Size());
            Flush();
        }

        m_Device = other.m_Device;
        m_FramesDelay = other.m_FramesDelay;
        m_DeletionQueue = std::move(other.m_DeletionQueue);
        m_MemoryAllocator = other.m_MemoryAllocator;

        other.m_Device = VK_NULL_HANDLE;
        other.m_FramesDelay = 0;
        other.m_MemoryAllocator = nullptr;

        return *this;
    }

    void DeleteQueue::Update()
    {
        if (m_DeletionQueue.Empty())
            return;

        for (size_t i = 0; i < m_DeletionQueue.Size(); )
        {
            DeletionItem& item = m_DeletionQueue[i];
            
            if (item.FramesToWait == 0)
            {
                DestroyObject(item);
                
                // Remove the item by shifting all remaining items forward
                for (size_t j = i; j < m_DeletionQueue.Size() - 1; j++)
                {
                    m_DeletionQueue[j] = VulkanHelper::Move(m_DeletionQueue[j + 1]);
                }
                m_DeletionQueue.PopBack();
                
                // Don't increment i since the item was removed from this index
            }
            else
            {
                --item.FramesToWait;
                i++; // Only increment when the item is kept
            }
        }
    }

    void DeleteQueue::QueueForDeletion(VkBuffer buffer, uint32_t framesToWait)
    {
        if (buffer != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::BUFFER, (uint64_t)buffer, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkImage image, uint32_t framesToWait)
    {
        if (image != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::IMAGE, (uint64_t)image, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkImageView imageView, uint32_t framesToWait)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::IMAGE_VIEW, (uint64_t)imageView, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkSampler sampler, uint32_t framesToWait)
    {
        if (sampler != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::SAMPLER, (uint64_t)sampler, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkPipeline pipeline, uint32_t framesToWait)
    {
        if (pipeline != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::PIPELINE, (uint64_t)pipeline, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkPipelineLayout pipelineLayout, uint32_t framesToWait)
    {
        if (pipelineLayout != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::PIPELINE_LAYOUT, (uint64_t)pipelineLayout, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkDescriptorSetLayout descriptorSetLayout, uint32_t framesToWait)
    {
        if (descriptorSetLayout != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::DESCRIPTOR_SET_LAYOUT, (uint64_t)descriptorSetLayout, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkDescriptorPool descriptorPool, uint32_t framesToWait)
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::DESCRIPTOR_POOL, (uint64_t)descriptorPool, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkCommandPool commandPool, uint32_t framesToWait)
    {
        if (commandPool != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::COMMAND_POOL, (uint64_t)commandPool, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkFence fence, uint32_t framesToWait)
    {
        if (fence != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::FENCE, (uint64_t)fence, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkSemaphore semaphore, uint32_t framesToWait)
    {
        if (semaphore != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::SEMAPHORE, (uint64_t)semaphore, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkSwapchainKHR swapchain, uint32_t framesToWait)
    {
        if (swapchain != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::SWAPCHAIN, (uint64_t)swapchain, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VkShaderModule shaderModule, uint32_t framesToWait)
    {
        if (shaderModule != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::SHADER_MODULE, (uint64_t)shaderModule, framesToWait);
        }
    }

    void DeleteQueue::QueueForDeletion(VulkanMemoryAllocator::BufferAllocation allocation, uint32_t framesToWait)
    {
        if (allocation.Buffer != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::BUFFER_ALLOCATION, (uint64_t)allocation.Buffer, framesToWait, allocation.Allocation);
        }
    }

    void DeleteQueue::QueueForDeletion(VulkanMemoryAllocator::ImageAllocation allocation, uint32_t framesToWait)
    {
        if (allocation.image != VK_NULL_HANDLE)
        {
            m_DeletionQueue.EmplaceBack(ObjectType::IMAGE_ALLOCATION, (uint64_t)allocation.image, framesToWait, allocation.Allocation);
        }
    }

    void DeleteQueue::Flush()
    {
        if (m_Device == nullptr)
        {
            VH_LOG_ERROR("DeleteQueue::Flush: Device is null, cannot destroy objects");
            return;
        }

        VH_LOG_INFO("Flushing DeleteQueue with {} pending objects", m_DeletionQueue.Size());

        for (const auto& item : m_DeletionQueue)
        {
            DestroyObject(item);
        }
        
        m_DeletionQueue.Clear();
    }

    void DeleteQueue::DestroyObject(const DeletionItem& item)
    {
        if (m_Device == nullptr)
        {
            VH_LOG_ERROR("DeleteQueue::DestroyObject: Device is null");
            return;
        }

        switch (item.Type)
        {
            case ObjectType::BUFFER:
                vkDestroyBuffer(m_Device, (VkBuffer)item.Handle, nullptr);
                break;

            case ObjectType::IMAGE:
                vkDestroyImage(m_Device, (VkImage)item.Handle, nullptr);
                break;

            case ObjectType::IMAGE_VIEW:
                vkDestroyImageView(m_Device, (VkImageView)item.Handle, nullptr);
                break;

            case ObjectType::SAMPLER:
                vkDestroySampler(m_Device, (VkSampler)item.Handle, nullptr);
                break;

            case ObjectType::PIPELINE:
                vkDestroyPipeline(m_Device, (VkPipeline)item.Handle, nullptr);
                break;

            case ObjectType::PIPELINE_LAYOUT:
                vkDestroyPipelineLayout(m_Device, (VkPipelineLayout)item.Handle, nullptr);
                break;

            case ObjectType::DESCRIPTOR_SET_LAYOUT:
                vkDestroyDescriptorSetLayout(m_Device, (VkDescriptorSetLayout)item.Handle, nullptr);
                break;

            case ObjectType::DESCRIPTOR_POOL:
                vkDestroyDescriptorPool(m_Device, (VkDescriptorPool)item.Handle, nullptr);
                break;

            case ObjectType::COMMAND_POOL:
                vkDestroyCommandPool(m_Device, (VkCommandPool)item.Handle, nullptr);
                break;

            case ObjectType::FENCE:
                vkDestroyFence(m_Device, (VkFence)item.Handle, nullptr);
                break;

            case ObjectType::SEMAPHORE:
                vkDestroySemaphore(m_Device, (VkSemaphore)item.Handle, nullptr);
                break;

            case ObjectType::SWAPCHAIN:
                vkDestroySwapchainKHR(m_Device, (VkSwapchainKHR)item.Handle, nullptr);
                break;

            case ObjectType::SHADER_MODULE:
                vkDestroyShaderModule(m_Device, (VkShaderModule)item.Handle, nullptr);
                break;

            case ObjectType::BUFFER_ALLOCATION:
                {
                    VulkanMemoryAllocator::BufferAllocation allocation{};
                    allocation.Buffer = (VkBuffer)item.Handle;
                    allocation.Allocation = item.Allocation;
                    m_MemoryAllocator->DeallocateBuffer(allocation);
                }
                break;

            case ObjectType::IMAGE_ALLOCATION:
                {
                    VulkanMemoryAllocator::ImageAllocation allocation{};
                    allocation.image = (VkImage)item.Handle;
                    allocation.Allocation = item.Allocation;
                    m_MemoryAllocator->DeallocateImage(allocation);
                }
                break;

            default:
                VH_ASSERT(false, "DeleteQueue::DestroyObject: Unknown object type");
                break;
        }
    }
}
