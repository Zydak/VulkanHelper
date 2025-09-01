#include "SamplerImpl.h"
#include "DeviceImpl.h"

#include "Core/Move.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    Expected<SharedPtr<Sampler::Impl>, VHResult> Sampler::Impl::New(
        const SharedPtr<Device::Impl>& device,
        Sampler::AddressMode addressMode,
        Sampler::Filter minFilter,
        Sampler::Filter magFilter,
        Sampler::MipmapMode mipmapMode)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = static_cast<VkFilter>(magFilter);
        samplerInfo.minFilter = static_cast<VkFilter>(minFilter);
        samplerInfo.addressModeU = static_cast<VkSamplerAddressMode>(addressMode);
        samplerInfo.addressModeV = static_cast<VkSamplerAddressMode>(addressMode);
        samplerInfo.addressModeW = static_cast<VkSamplerAddressMode>(addressMode);
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = static_cast<VkSamplerMipmapMode>(mipmapMode);
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkSampler sampler;
        VkResult res = vkCreateSampler(device->GetDevice(), &samplerInfo, nullptr, &sampler);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create Vulkan sampler");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(device, sampler)));
    }

    Sampler::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device), m_Sampler(other.m_Sampler)
    {
        other.m_Device = nullptr;
        other.m_Sampler = VK_NULL_HANDLE;
    }

    Sampler::Impl& Sampler::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (m_Sampler != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan Sampler Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Sampler);
            m_Sampler = VK_NULL_HANDLE;
            m_Device = nullptr;
        }

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Sampler = other.m_Sampler;
        other.m_Sampler = VK_NULL_HANDLE;

        return *this;
    }

    Sampler::Impl::~Impl()
    {
        if (m_Sampler != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan Sampler Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Sampler);
            m_Sampler = VK_NULL_HANDLE;
            m_Device = nullptr;
        }
    }

    //
    //  Forward functions
    //

    Expected<Sampler, VHResult> Sampler::New(const Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Sampler Implementation");

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.AddressMode,
            config.MinFilter,
            config.MagFilter,
            config.MipmapMode
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Sampler::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    Sampler::Sampler()
        : m_Impl(nullptr)
    {
    }

    Sampler::Sampler(const Sampler& other)
        : m_Impl(other.m_Impl)
    {
    }

    Sampler& Sampler::operator=(const Sampler& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;

        return *this;
    }

    Sampler::Sampler(Sampler&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {
    }

    Sampler& Sampler::operator=(Sampler&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Sampler::~Sampler()
    {
    }

    Sampler::Sampler(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
    }
}
