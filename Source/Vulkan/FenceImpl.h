#pragma once
#include "Vulkan/Fence.h"
#include "Vulkan/Device.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Fence::Impl
    {
    public:
        [[nodiscard]] static Expected<Impl, VHResult> New(Device::Impl* device, bool startSignaled);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Fence* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Fence CreatePublicInterface(Impl&& impl) { return Fence(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<Fence> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new Fence(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        void Wait();
        void Reset();

        [[nodiscard]] inline VkFence GetFenceHandle() const { return m_Fence; }
    private:
        Impl(Device::Impl* device, VkFence fence)
            : m_Device(device), m_Fence(fence)
        {}

        Device::Impl* m_Device;
        VkFence m_Fence;
    };
}