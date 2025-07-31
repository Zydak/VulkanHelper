#pragma once
#include "Vulkan/Fence.h"
#include "Vulkan/Device.h"

typedef struct VkFence_T* VkFence;

namespace VulkanHelper
{
    class Fence::Impl
    {
    public:
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(const Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Fence* publicInterface) { return publicInterface->m_Impl.Get(); }

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