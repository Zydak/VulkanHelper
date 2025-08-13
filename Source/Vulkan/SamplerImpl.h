#pragma once
#include "Vulkan/Sampler.h"
#include "Vulkan/Device.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Sampler::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            Sampler::AddressMode addressMode,
            Sampler::Filter minFilter,
            Sampler::Filter magFilter,
            Sampler::MipmapMode mipmapMode);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Sampler& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Sampler CreatePublicInterface(const SharedPtr<Impl>& impl) { return Sampler(impl); }

        [[nodiscard]] inline VkSampler GetSampler() const { return m_Sampler; }
    private:
        Impl(const SharedPtr<Device::Impl>& device, VkSampler sampler)
            : m_Device(device), m_Sampler(sampler)
        {}

        SharedPtr<Device::Impl> m_Device;
        VkSampler m_Sampler;
    };
}
