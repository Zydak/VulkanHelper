#pragma once
#include "Pch.h"
#include "DescriptorSet.h"

#include "Vulkan/Device.h"
#include "Logger/Logger.h"
#include "Pipeline.h"

VulkanHelper::ResultCode VulkanHelper::DescriptorSet::Init(const CreateInfo& createInfo)
{
	m_Device = createInfo.Device;
	m_Pool = createInfo.Pool;

	ResultCode res = m_Layout.Init({ m_Device, createInfo.Bindings });
	if (res != ResultCode::Success)
		return res;

	res = m_Writer.Init({ m_Device, &m_Layout });
	if (res != ResultCode::Success)
		return res;

	return m_Pool->AllocateDescriptorSets(m_Layout.GetDescriptorSetLayoutHandle(), &m_DescriptorSetHandle);
}

VulkanHelper::DescriptorSet::~DescriptorSet()
{
	Destroy();
}

VulkanHelper::DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
{
	if (this == &other)
		return;

	Destroy();

	Move(std::move(other));
}

VulkanHelper::DescriptorSet& VulkanHelper::DescriptorSet::operator=(DescriptorSet&& other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	Move(std::move(other));

	return *this;
}

void VulkanHelper::DescriptorSet::Move(DescriptorSet&& other) noexcept
{
	m_Device = other.m_Device;
	m_Pool = other.m_Pool;
	m_DescriptorSetHandle = other.m_DescriptorSetHandle;
	m_Layout = std::move(other.m_Layout);
	m_Writer = std::move(other.m_Writer);
}

void VulkanHelper::DescriptorSet::Destroy()
{
	m_DescriptorSetHandle = VK_NULL_HANDLE;
}

void VulkanHelper::DescriptorSet::Bind(uint32_t set, Pipeline* pipeline, VkCommandBuffer cmdBuffer)
{
	VH_ASSERT(m_DescriptorSetHandle != VK_NULL_HANDLE, "DescriptorSet Not Initialized!");

	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
	if (pipeline->GetPipelineType() == Pipeline::PipelineType::Graphics)
		bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	else if (pipeline->GetPipelineType() == Pipeline::PipelineType::Compute)
		bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	else if (pipeline->GetPipelineType() == Pipeline::PipelineType::RayTracing)
		bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

	vkCmdBindDescriptorSets(
		cmdBuffer,
		bindPoint,
		pipeline->GetPipelineLayout(),
		set,
		1,
		&m_DescriptorSetHandle,
		0,
		nullptr
	);
}

void VulkanHelper::DescriptorSet::AddBuffer(uint32_t binding, uint32_t arrayElement, const VkDescriptorBufferInfo& bufferInfo)
{
	VH_ASSERT(m_DescriptorSetHandle != VK_NULL_HANDLE, "DescriptorSet Not Initialized!");
	VH_ASSERT(m_Layout.GetDescriptorSetLayoutBindings().size() > binding, "There are only {0} bindings, you're trying to write to {1}", m_Layout.GetDescriptorSetLayoutBindings().size(), binding);

	VH_ASSERT(
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		"Wrong Binding Type! Type inside layout is: {}", (uint32_t)m_Layout.GetDescriptorSetLayoutBindings()[binding].Type
	);

	VH_ASSERT(
		m_Layout.GetDescriptorSetLayoutBindings()[binding].DescriptorsCount > arrayElement,
		"array element is out of bounds! arrayElement: {0}, descriptorCount inside layout: {0}",
		arrayElement, m_Layout.GetDescriptorSetLayoutBindings()[binding].DescriptorsCount
	);

	m_Writer.WriteBuffer(binding, arrayElement, &bufferInfo);
}

void VulkanHelper::DescriptorSet::AddImage(uint32_t binding, uint32_t arrayElement, const VkDescriptorImageInfo& imageInfo)
{
	VH_ASSERT(m_DescriptorSetHandle != VK_NULL_HANDLE, "DescriptorSet Not Initialized!");
	VH_ASSERT(m_Layout.GetDescriptorSetLayoutBindings().size() > binding, "There are only {0} bindings, you're trying to write to {1}", m_Layout.GetDescriptorSetLayoutBindings().size(), binding);

	VH_ASSERT(
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_SAMPLER,
		"Wrong Binding Type! Type inside layout is: {}", (uint32_t)m_Layout.GetDescriptorSetLayoutBindings()[binding].Type
	);

	VH_ASSERT(
		m_Layout.GetDescriptorSetLayoutBindings()[binding].DescriptorsCount > arrayElement,
		"array element is out of bounds! arrayElement: {0}, descriptorCount inside layout: {0}",
		arrayElement, m_Layout.GetDescriptorSetLayoutBindings()[binding].DescriptorsCount
	);

	m_Writer.WriteImageOrSampler(binding, arrayElement, &imageInfo);
}

void VulkanHelper::DescriptorSet::AddAccelerationStructure(uint32_t binding, uint32_t arrayElement, const VkWriteDescriptorSetAccelerationStructureKHR& asInfo)
{
	VH_ASSERT(m_DescriptorSetHandle != VK_NULL_HANDLE, "DescriptorSet Not Initialized!");
	VH_ASSERT(m_Layout.GetDescriptorSetLayoutBindings().size() > binding, "There are only {0} bindings, you're trying to write to {1}", m_Layout.GetDescriptorSetLayoutBindings().size(), binding);

	VH_ASSERT(
		m_Layout.GetDescriptorSetLayoutBindings()[binding].Type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR ||
		"Wrong Binding Type! Type inside layout is: {}", (uint32_t)m_Layout.GetDescriptorSetLayoutBindings()[binding].Type
	);

	VH_ASSERT(
		m_Layout.GetDescriptorSetLayoutBindings()[binding].DescriptorsCount > arrayElement,
		"array element is out of bounds! arrayElement: {0}, descriptorCount inside layout: {0}",
		arrayElement, m_Layout.GetDescriptorSetLayoutBindings()[binding].DescriptorsCount
	);

	m_Writer.WriteAs(binding, arrayElement, &asInfo);
}

void VulkanHelper::DescriptorSet::Write()
{
	m_Writer.WriteToSet(m_DescriptorSetHandle);
}
