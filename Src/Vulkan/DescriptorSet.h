#pragma once
#include "Pch.h"

#include "vulkan/vulkan.h"
#include "Vulkan/ErrorCodes.h"

#include "Vulkan/Descriptors/DescriptorSetLayout.h"
#include "Vulkan/Descriptors/DescriptorPool.h"
#include "Vulkan/Descriptors/DescriptorWriter.h"

namespace VulkanHelper
{
	class Device;
	class Pipeline;

	class DescriptorSet
	{
	public:
		struct CreateInfo
		{
			Device* Device = nullptr;
			DescriptorPool* Pool = nullptr;
			std::vector<DescriptorSetLayout::Binding> Bindings;
		};

		[[nodiscard]] ResultCode Init(const CreateInfo& createInfo);
		DescriptorSet() = default;
		~DescriptorSet();

		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;
		DescriptorSet(DescriptorSet&& other) noexcept;
		DescriptorSet& operator=(DescriptorSet&& other) noexcept;

		void Bind(uint32_t set, Pipeline* pipeline, VkCommandBuffer cmdBuffer);

		void AddBuffer(uint32_t binding, uint32_t arrayElement, const VkDescriptorBufferInfo& bufferInfo);
		void AddImage(uint32_t binding, uint32_t arrayElement, const VkDescriptorImageInfo& imageInfo);
		void AddAccelerationStructure(uint32_t binding, uint32_t arrayElement, const VkWriteDescriptorSetAccelerationStructureKHR& asInfo);

		void Write();

	public:
		[[nodiscard]] inline VkDescriptorSet GetDescriptorSetHandle() const { return m_DescriptorSetHandle; }
		[[nodiscard]] inline DescriptorSetLayout* GetLayout() { return &m_Layout; }

	private:
		void Move(DescriptorSet&& other) noexcept;
		void Destroy();

		Device* m_Device = nullptr;
		DescriptorPool* m_Pool = nullptr;
		VkDescriptorSet m_DescriptorSetHandle = VK_NULL_HANDLE;
		DescriptorWriter m_Writer;
		DescriptorSetLayout m_Layout;
	};
}