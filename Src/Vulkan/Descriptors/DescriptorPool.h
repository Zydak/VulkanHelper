#pragma once
#include "Pch.h"

#include "Vulkan/ErrorCodes.h"
#include "vulkan/vulkan.h"

namespace VulkanHelper
{
	class Device;

	class DescriptorPool
	{
	public:

		struct PoolSize
		{
			VkDescriptorType PoolType = VkDescriptorType::VK_DESCRIPTOR_TYPE_MAX_ENUM;
			uint32_t DescriptorCount = -1;

			operator bool() const
			{
				return (PoolType != VK_DESCRIPTOR_TYPE_MAX_ENUM) && (DescriptorCount != -1);
			}
		};

		struct CreateInfo
		{
			Device* Device = nullptr;
			std::vector<PoolSize> PoolSizes;
			uint32_t MaxSets = 0;
			VkDescriptorPoolCreateFlags PoolFlags = 0;
		};

		[[nodiscard]] ResultCode Init(const CreateInfo& createInfo);

		DescriptorPool() = default;
		~DescriptorPool();

		DescriptorPool(const DescriptorPool&) = delete;
		DescriptorPool& operator=(const DescriptorPool&) = delete;

		DescriptorPool(DescriptorPool&& other) noexcept;
		DescriptorPool& operator=(DescriptorPool&& other) noexcept;

		[[nodiscard]] ResultCode AllocateDescriptorSets(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet* descriptor);

		[[nodiscard]] inline VkDescriptorPool GetDescriptorPoolHandle(uint32_t index = 0) { return m_DescriptorPoolHandles[index]; }

		void ResetPool(uint32_t index = 0);

	private:
		[[nodiscard]] ResultCode CreateNewPool();

		Device* m_Device = nullptr;

		uint32_t m_CurrentPool = 0;
		std::vector<VkDescriptorPool> m_DescriptorPoolHandles;
		std::vector<PoolSize> m_PoolSizes;
		uint32_t m_MaxSets;
		VkDescriptorPoolCreateFlags m_PoolFlags;

		void Move(DescriptorPool&& other) noexcept;
		void Destroy();

		friend class DescriptorWriter;
	};

}