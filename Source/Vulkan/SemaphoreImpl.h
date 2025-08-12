#pragma once
#include "Vulkan/Semaphore.h"
#include "Vulkan/Device.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Semaphore::Impl
    {
    public:
        [[nodiscard]] static Expected<Impl, VHResult> New(Device::Impl* device);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Semaphore* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Semaphore CreatePublicInterface(Impl&& impl) { return Semaphore(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<Semaphore> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new Semaphore(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        [[nodiscard]] inline VkSemaphore GetSemaphore() const { return m_Semaphore; } 
    private:
        Impl(Device::Impl* device, VkSemaphore semaphore)
            : m_Device(device), m_Semaphore(semaphore)
        {}

        VulkanHelper::Device::Impl* m_Device;
        VkSemaphore m_Semaphore;
    };
}