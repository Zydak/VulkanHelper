#pragma once
#include "Vulkan/Device.h"
#include "InstanceImpl.h"
#include "VulkanMemoryAllocator.h"

typedef struct VkDevice_T* VkDevice;

namespace VulkanHelper
{
    class Device::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Config& config);
        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Device* publicInterface) { return publicInterface->m_Impl.Get(); }

        [[nodiscard]] inline VkDevice GetDevice() const { return m_Device; }
        [[nodiscard]] inline const PhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
        [[nodiscard]] inline QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
        void WaitUntilIdle() const;

        /**
         * @brief Allocate a buffer using the device's memory allocator.
         *
         * @param bufferInfo Vulkan buffer creation info.
         * @param allowCpuAccess Whether to create memory mappable or not.
         * @return Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> Buffer allocation on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowCpuAccess = false);

        /**
         * @brief Allocate an image using the device's memory allocator.
         *
         * @param imageInfo Vulkan image creation info.
         * @param allowCpuAccess Whether to create memory mappable or not.
         * @return Expected<VulkanMemoryAllocator::ImageAllocation, VHResult> Image allocation on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<VulkanMemoryAllocator::ImageAllocation, VHResult> AllocateImage(const VkImageCreateInfo& imageInfo, bool allowCpuAccess = false);

        /**
         * @brief Deallocate a previously allocated buffer.
         *
         * @param allocation Buffer allocation to deallocate.
         */
        void DeallocateBuffer(const VulkanMemoryAllocator::BufferAllocation& allocation);

        /**
         * @brief Deallocate a previously allocated image.
         *
         * @param allocation Image allocation to deallocate.
         */
        void DeallocateImage(const VulkanMemoryAllocator::ImageAllocation& allocation);

        /**
         * @brief Map buffer memory for CPU access.
         *
         * @param allocation Buffer allocation to map.
         * @return Expected<void*, VHResult> Pointer to mapped memory on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<void*, VHResult> MapBuffer(const VulkanMemoryAllocator::BufferAllocation& allocation);

        /**
         * @brief Unmap previously mapped buffer memory.
         *
         * @param allocation Buffer allocation to unmap.
         */
        void UnmapBuffer(const VulkanMemoryAllocator::BufferAllocation& allocation);
    private:

        Instance::Impl* m_Instance;
        VkDevice m_Device = nullptr;
        PhysicalDevice m_PhysicalDevice;
        QueueFamilyIndices m_QueueFamilyIndices = {};
        VulkanHelper::VulkanMemoryAllocator m_Allocator;

        explicit Impl(
            Instance::Impl* instance,
            VkDevice device,
            PhysicalDevice physicalDevice,
            QueueFamilyIndices&& indices,
            VulkanHelper::VulkanMemoryAllocator&& allocator
        )
            : m_Instance(instance)
            , m_Device(device)
            , m_PhysicalDevice(physicalDevice)
            , m_QueueFamilyIndices(Move(indices))
            , m_Allocator(Move(allocator))
        {}
        
        [[nodiscard]] static QueueFamilyIndices FindQueueFamilies(const PhysicalDevice& physicalDevice, const VulkanHelper::Vector<Window*>& windows);
    };
}