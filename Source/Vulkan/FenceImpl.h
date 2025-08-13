#pragma once
#include "Vulkan/Fence.h"
#include "Vulkan/Device.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Fence::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(const SharedPtr<Device::Impl>& device, bool startSignaled);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Fence& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Fence CreatePublicInterface(const SharedPtr<Impl>& impl) { return Fence(impl); }

        void Wait();
        void Reset();

        [[nodiscard]] inline VkFence GetFenceHandle() const { return m_Fence; }
    private:
        Impl(const SharedPtr<Device::Impl>& device, VkFence fence)
            : m_Device(device), m_Fence(fence)
        {}

        SharedPtr<Device::Impl> m_Device;
        VkFence m_Fence;
    };
}