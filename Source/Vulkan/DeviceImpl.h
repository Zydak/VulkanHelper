#pragma once
#include "Vulkan/Device.h"

#include "Window/WindowImpl.h"
#include "InstanceImpl.h"
#include "VulkanMemoryAllocator.h"

#include "PhysicalDeviceImpl.h"
#include "DeleteQueue.h"

namespace VulkanHelper
{
    class Device::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(PhysicalDevice::Impl physicalDevice, Vector<Window::Impl*>&& windows, Instance::Impl* instance, bool requestRTSupport = false);
        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Device* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Device CreatePublicInterface(VulkanHelper::UniquePtr<Impl>&& impl) { return Device(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline VkDevice GetDevice() const { return m_Device; }
        [[nodiscard]] inline const PhysicalDevice::Impl& GetPhysicalDevice() const { return m_PhysicalDevice; }
        [[nodiscard]] inline QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
        void WaitUntilIdle() const;

        /**
         * @brief Allocate a buffer using the device's memory allocator.
         *
         * @param bufferInfo Vulkan buffer creation info.
         * @param allowCpuAccess Whether to create memory mappable or not.
         * @return Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> Buffer allocation on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowCpuAccess = false, uint32_t alignment = 1);

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
         * @brief Map memory for CPU access.
         *
         * @param allocation Allocation to map.
         * @return Expected<void*, VHResult> Pointer to mapped memory on success, or error on failure.
         */
        [[nodiscard]] VulkanHelper::Expected<void*, VHResult> MapMemory(const VmaAllocation& allocation);

        /**
         * @brief Unmap previously mapped memory.
         *
         * @param allocation Allocation to unmap.
         */
        void UnmapMemory(const VmaAllocation& allocation);

        /**
         * @brief Get the delete queue for deferred object destruction.
         *
         * @return Reference to the delete queue.
         */
        [[nodiscard]] inline DeleteQueue& GetDeleteQueue() { return m_DeleteQueue; }

        /**
         * @brief Get the physical device properties.
         *
         * @return Reference to the physical device properties structure.
         */
        [[nodiscard]] inline const VkPhysicalDeviceProperties2& GetPhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }

        /**
         * @brief Get the ray tracing properties of the physical device.
         *
         * @return Reference to the ray tracing properties structure.
         */
        [[nodiscard]] inline const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingProperties() const { return m_RayTracingProperties; }

        /**
         * @brief Get the acceleration structure properties of the physical device.
         *
         * @return Reference to the acceleration structure properties structure.
         */
        [[nodiscard]] inline const VkPhysicalDeviceAccelerationStructurePropertiesKHR& GetAccelerationStructureProperties() const { return m_AccelerationStructureProperties; }
    private:

        /**
         * @brief Initialize the delete queue after the device is constructed.
         */
        void InitializeDeleteQueue(uint32_t framesDelay);

        Instance::Impl* m_Instance;
        VkDevice m_Device = nullptr;
        PhysicalDevice::Impl m_PhysicalDevice;
        QueueFamilyIndices m_QueueFamilyIndices = {};
        VulkanHelper::VulkanMemoryAllocator m_Allocator;
        DeleteQueue m_DeleteQueue;

        VkPhysicalDeviceProperties2 m_PhysicalDeviceProperties = {};
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_RayTracingProperties = {};
        VkPhysicalDeviceAccelerationStructurePropertiesKHR m_AccelerationStructureProperties = {};

        explicit Impl(
            Instance::Impl* instance,
            VkDevice device,
            PhysicalDevice::Impl&& physicalDevice,
            QueueFamilyIndices&& indices,
            VulkanHelper::VulkanMemoryAllocator&& allocator,
            VkPhysicalDeviceProperties2&& physicalDeviceProperties,
            VkPhysicalDeviceRayTracingPipelinePropertiesKHR&& rayTracingProperties,
            VkPhysicalDeviceAccelerationStructurePropertiesKHR&& accelerationStructureProperties = {}
        )
            : m_Instance(instance)
            , m_Device(device)
            , m_PhysicalDevice(Move(physicalDevice))
            , m_QueueFamilyIndices(Move(indices))
            , m_Allocator(Move(allocator))
            , m_DeleteQueue(VK_NULL_HANDLE, nullptr, 0) // Will be properly initialized later
            , m_PhysicalDeviceProperties(Move(physicalDeviceProperties))
            , m_RayTracingProperties(Move(rayTracingProperties))
            , m_AccelerationStructureProperties(Move(accelerationStructureProperties))
        {}

        [[nodiscard]] static QueueFamilyIndices FindQueueFamilies(const PhysicalDevice::Impl& physicalDevice, const VulkanHelper::Vector<Window::Impl*>& windows);
    };
}