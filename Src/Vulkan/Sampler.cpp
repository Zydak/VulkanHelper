#include "Pch.h"
#include "Logger/Logger.h"

#include "Sampler.h"

#include "Device.h"

namespace VulkanHelper
{

	void Sampler::Init(const SamplerInfo& samplerInfo)
	{
		Destroy();

		m_Device = samplerInfo.Device;

		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = samplerInfo.FilterMode;
		samplerCreateInfo.minFilter = samplerInfo.FilterMode;
		samplerCreateInfo.mipmapMode = samplerInfo.MipmapMode;
		samplerCreateInfo.addressModeU = samplerInfo.AddressMode;
		samplerCreateInfo.addressModeV = samplerInfo.AddressMode;
		samplerCreateInfo.addressModeW = samplerInfo.AddressMode;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCreateInfo.minLod = 0;
		samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCreateInfo.maxAnisotropy = m_Device->GetPhysicalDeviceProperties().properties.limits.maxSamplerAnisotropy;
		samplerCreateInfo.anisotropyEnable = VK_FALSE;
		samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
		samplerCreateInfo.compareEnable = VK_FALSE;
		VH_RETURN_ASSERT(vkCreateSampler(m_Device->GetHandle(), &samplerCreateInfo, nullptr, &m_SamplerHandle),
			"failed to create texture sampler"
		);
	}

	Sampler::~Sampler()
	{
		Destroy();
	}

	void Sampler::Destroy()
	{
		if (m_SamplerHandle == VK_NULL_HANDLE)
			return;

		vkDestroySampler(m_Device->GetHandle(), m_SamplerHandle, nullptr);

		Reset();
	}

	void Sampler::Move(Sampler&& other) noexcept
	{
		m_Device = other.m_Device;

		m_SamplerHandle = other.m_SamplerHandle;

		other.Reset();
	}

	Sampler::Sampler(const SamplerInfo& samplerInfo)
	{
		Init(samplerInfo);
	}

	Sampler::Sampler(Sampler&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	Sampler& Sampler::operator=(Sampler&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	void Sampler::Reset()
	{
		m_SamplerHandle = VK_NULL_HANDLE;
	}

}