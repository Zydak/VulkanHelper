#pragma once
#include "vulkan/vulkan_core.h"

namespace VulkanHelper
{

	class Device;

	struct SamplerInfo
	{
		Device* Device = nullptr;
		VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		VkFilter FilterMode = VK_FILTER_LINEAR;
		VkSamplerMipmapMode MipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	};

	class Sampler
	{
	public:
		void Init(const SamplerInfo& samplerInfo);

		Sampler() = default;
		Sampler(const SamplerInfo& samplerInfo);
		~Sampler();

		Sampler(const Sampler& other) = delete;
		Sampler& operator=(const Sampler& other) = delete;
		Sampler(Sampler&& other) noexcept;
		Sampler& operator=(Sampler&& other) noexcept;

		inline VkSampler GetSamplerHandle() const { return m_SamplerHandle; }

	private:
		Device* m_Device = nullptr;
		VkSampler m_SamplerHandle = VK_NULL_HANDLE;

		void Destroy();
		void Move(Sampler&& other) noexcept;
		void Reset();
	};

}