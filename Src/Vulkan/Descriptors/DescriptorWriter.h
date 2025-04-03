#pragma once

#include "Vulkan/Device.h"
#include "DescriptorSetLayout.h"
#include "DescriptorPool.h"

namespace VulkanHelper
{
	class DescriptorWriter
	{
	public:
		struct CreateInfo
		{
			Device* Device = nullptr;
			DescriptorSetLayout* SetLayout = nullptr;
		};

		[[nodiscard]] ResultCode Init(const CreateInfo& createInfo);

		DescriptorWriter() = default;
		~DescriptorWriter();

		DescriptorWriter(const DescriptorWriter&) = delete;
		DescriptorWriter& operator=(const DescriptorWriter&) = delete;

		DescriptorWriter(DescriptorWriter&&) noexcept;
		DescriptorWriter& operator=(DescriptorWriter&&) noexcept;

		void WriteBuffer(uint32_t binding, uint32_t arrayElement, const VkDescriptorBufferInfo* bufferInfo);
		void WriteAs(uint32_t binding, uint32_t arrayElement, const VkWriteDescriptorSetAccelerationStructureKHR* asInfo);
		void WriteImageOrSampler(uint32_t binding, uint32_t arrayElement, const VkDescriptorImageInfo* imageInfo);

		void WriteToSet(VkDescriptorSet set);

	private:
		Device* m_Device = nullptr;
		DescriptorSetLayout* m_SetLayout = nullptr;
		std::vector<VkWriteDescriptorSet> m_Writes;

		void Destroy();
		void Move(DescriptorWriter&& other) noexcept;
	};
}