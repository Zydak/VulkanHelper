#include "Pch.h"
#include "DescriptorSetLayout.h"

#include "Vulkan/Device.h"
#include "Logger/Logger.h"
#include "Vulkan/DeleteQueue.h"

namespace VulkanHelper
{
	ResultCode DescriptorSetLayout::Init(const CreateInfo& createInfo)
	{
		Destroy();

		m_Device = createInfo.Device;
		m_Bindings = createInfo.Bindings;

		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};

		for (int i = 0; i < m_Bindings.size(); i++)
		{
			VH_ASSERT(m_Bindings[i], "Incorrectly initialized binding in array!");

			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = m_Bindings[i].BindingNumber;
			layoutBinding.descriptorType = m_Bindings[i].Type;
			layoutBinding.descriptorCount = m_Bindings[i].DescriptorsCount;
			layoutBinding.stageFlags = m_Bindings[i].StageFlags;

			setLayoutBindings.push_back(layoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = uint32_t(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		return (ResultCode)vkCreateDescriptorSetLayout(m_Device->GetHandle(), &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayoutHandle);
	}

	void DescriptorSetLayout::Move(DescriptorSetLayout&& other) noexcept
	{
		m_Device = other.m_Device;
		m_DescriptorSetLayoutHandle = std::move(other.m_DescriptorSetLayoutHandle);
		m_Bindings = std::move(other.m_Bindings);
	}

	void DescriptorSetLayout::Destroy()
	{
		if (m_DescriptorSetLayoutHandle == VK_NULL_HANDLE)
			return;

		DeleteQueue::DeleteDescriptorSetLayout(*this);
	}

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	DescriptorSetLayout::~DescriptorSetLayout()
	{
		Destroy();
	}
}