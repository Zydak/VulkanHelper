#pragma once
#include "Vulkan/Semaphore.h"
#include "Vulkan/Device.h"

typedef struct VkSemaphore_T* VkSemaphore;

namespace VulkanHelper
{
    class Semaphore::Impl
    {
    public:
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(const Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline VkSemaphore GetSemaphore() const { return m_Semaphore; } 
    private:
        Impl(Device::Impl* device, VkSemaphore semaphore)
            : m_Device(device), m_Semaphore(semaphore)
        {}

        VulkanHelper::Device::Impl* m_Device;
        VkSemaphore m_Semaphore;
    };
}