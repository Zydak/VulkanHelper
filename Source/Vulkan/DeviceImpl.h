#pragma once
#include "Vulkan/Device.h"

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
    private:

        Impl(PhysicalDevice physicalDevice, VkDevice device, QueueFamilyIndices&& indices)
            : m_PhysicalDevice(physicalDevice), m_Device(device), m_QueueFamilyIndices(VulkanHelper::Move(indices))
            {}

        PhysicalDevice m_PhysicalDevice;
        VkDevice m_Device = nullptr;
        QueueFamilyIndices m_QueueFamilyIndices = {};
        
        [[nodiscard]] static QueueFamilyIndices FindQueueFamilies(const PhysicalDevice& physicalDevice, const VulkanHelper::Vector<Window*>& windows);
    };
}