#pragma once
#include "Vulkan/Semaphore.h"
#include "Vulkan/Device.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Semaphore::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(const SharedPtr<Device::Impl>& device);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Semaphore& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Semaphore CreatePublicInterface(const SharedPtr<Impl>& impl) { return Semaphore(impl); }

        [[nodiscard]] inline VkSemaphore GetSemaphore() const { return m_Semaphore; } 
    private:
        Impl(const SharedPtr<Device::Impl>& device, VkSemaphore semaphore)
            : m_Device(device), m_Semaphore(semaphore)
        {}

        SharedPtr<Device::Impl> m_Device;
        VkSemaphore m_Semaphore;
    };
}