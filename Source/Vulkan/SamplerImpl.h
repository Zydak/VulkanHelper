#pragma once
#include "Vulkan/Sampler.h"
#include "Vulkan/Device.h"

typedef struct VkSampler_T* VkSampler;

namespace VulkanHelper
{
    class Sampler::Impl
    {
    public:
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(Device::Impl* device, 
            Sampler::AddressMode addressMode, 
            Sampler::Filter minFilter,
            Sampler::Filter magFilter,
            Sampler::MipmapMode mipmapMode);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Sampler* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Sampler CreatePublicInterface(UniquePtr<Impl>&& impl) { return Sampler(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline VkSampler GetSampler() const { return m_Sampler; }
    private:
        Impl(Device::Impl* device, VkSampler sampler)
            : m_Device(device), m_Sampler(sampler)
        {}

        Device::Impl* m_Device;
        VkSampler m_Sampler;
    };
}
