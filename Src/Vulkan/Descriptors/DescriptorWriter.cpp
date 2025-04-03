#include "pch.h"
#include "DescriptorWriter.h"

#include "Vulkan/Device.h"
#include "Logger/Logger.h"

namespace VulkanHelper
{

	ResultCode DescriptorWriter::Init(const CreateInfo& createInfo)
	{
		Destroy();

		m_SetLayout = createInfo.SetLayout;
		m_Device = createInfo.Device;

		return ResultCode::Success;
	}

	void DescriptorWriter::Destroy()
	{
		m_SetLayout = nullptr;
	}

	void DescriptorWriter::Move(DescriptorWriter&& other) noexcept
	{
		m_Device = other.m_Device;
		m_SetLayout = other.m_SetLayout;
		m_Writes = std::move(other.m_Writes);
	}

	DescriptorWriter::DescriptorWriter(DescriptorWriter&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	DescriptorWriter& DescriptorWriter::operator=(DescriptorWriter&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	DescriptorWriter::~DescriptorWriter()
	{
		Destroy();
	}

	void DescriptorWriter::WriteBuffer(uint32_t binding, uint32_t arrayElement, const VkDescriptorBufferInfo* bufferInfo)
	{
		VH_ASSERT(m_SetLayout != nullptr, "DescriptorWriter Not Initialized!");

		VH_ASSERT(m_SetLayout->m_Bindings.size() > binding, "Layout does not contain specified binding: {0}", binding);

		auto& bindingDescription = m_SetLayout->m_Bindings[binding];

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.Type;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;
		write.dstArrayElement = arrayElement;

		m_Writes.push_back(write);
	}

	void DescriptorWriter::WriteAs(uint32_t binding, uint32_t arrayElement, const VkWriteDescriptorSetAccelerationStructureKHR* asInfo)
	{
		VH_ASSERT(m_SetLayout != nullptr, "DescriptorWriter Not Initialized!");

		VH_ASSERT(m_SetLayout->m_Bindings.size() > binding, "Layout does not contain specified binding: {0}", binding);

		auto& bindingDescription = m_SetLayout->m_Bindings[binding];

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.Type;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.pNext = asInfo;
		write.dstArrayElement = arrayElement;

		m_Writes.push_back(write);
	}

	void DescriptorWriter::WriteImageOrSampler(uint32_t binding, uint32_t arrayElement, const VkDescriptorImageInfo* imageInfo)
	{
		VH_ASSERT(m_SetLayout != nullptr, "DescriptorWriter Not Initialized!");

		VH_ASSERT(m_SetLayout->m_Bindings.size() > binding, "Layout does not contain specified binding: {0}", binding);

		auto& bindingDescription = m_SetLayout->m_Bindings[binding];

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.Type;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = 1;
		write.dstArrayElement = arrayElement;

		m_Writes.push_back(write);
	}

	void DescriptorWriter::WriteToSet(VkDescriptorSet set)
	{
		VH_ASSERT(m_SetLayout != nullptr, "DescriptorWriter Not Initialized!");

		for (auto& write : m_Writes)
		{
			write.dstSet = set;
		}

		vkUpdateDescriptorSets(m_Device->GetHandle(), (uint32_t)m_Writes.size(), m_Writes.data(), 0, nullptr);

		m_Writes.clear();
	}

}