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
        [[nodiscard]] static Expected<Impl, VHResult> New(PhysicalDevice::Impl physicalDevice, const Vector<Window::Impl*>& windows, Instance::Impl* instance, bool requestRTSupport = false);
        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Device* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Device CreatePublicInterface(Impl&& impl) { return Device(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<Device> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new Device(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        [[nodiscard]] inline VkDevice GetDevice() const { return m_Device; }
        [[nodiscard]] inline const PhysicalDevice::Impl& GetPhysicalDevice() const { return m_PhysicalDevice; }
        [[nodiscard]] inline QueueFamilyIndices GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }
        void WaitUntilIdle() const;

        [[nodiscard]] VulkanHelper::Expected<VulkanMemoryAllocator::BufferAllocation, VHResult> AllocateBuffer(const VkBufferCreateInfo& bufferInfo, bool allowCpuAccess = false, uint32_t alignment = 1);

        [[nodiscard]] VulkanHelper::Expected<VulkanMemoryAllocator::ImageAllocation, VHResult> AllocateImage(const VkImageCreateInfo& imageInfo, bool allowCpuAccess = false);

        void DeallocateBuffer(const VulkanMemoryAllocator::BufferAllocation& allocation);

        void DeallocateImage(const VulkanMemoryAllocator::ImageAllocation& allocation);

        [[nodiscard]] VulkanHelper::Expected<void*, VHResult> MapMemory(const VmaAllocation& allocation);

        void UnmapMemory(const VmaAllocation& allocation);

        [[nodiscard]] inline DeleteQueue& GetDeleteQueue() { return m_DeleteQueue; }

        [[nodiscard]] inline const VkPhysicalDeviceProperties2& GetPhysicalDeviceProperties() const { return m_PhysicalDeviceProperties; }
        [[nodiscard]] inline const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& GetRayTracingProperties() const { return m_RayTracingProperties; }
        [[nodiscard]] inline const VkPhysicalDeviceAccelerationStructurePropertiesKHR& GetAccelerationStructureProperties() const { return m_AccelerationStructureProperties; }

        void InitializeDeleteQueue(uint32_t framesDelay);
    private:

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