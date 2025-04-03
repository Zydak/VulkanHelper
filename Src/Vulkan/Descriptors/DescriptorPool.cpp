#include "Pch.h"
#include "DescriptorPool.h"

#include "Vulkan/Device.h"
#include "Logger/Logger.h"

namespace VulkanHelper
{
	ResultCode DescriptorPool::Init(const CreateInfo& createInfo)
	{
		Destroy();

		m_DescriptorPoolHandles.resize(1);

		m_Device = createInfo.Device;
		m_PoolSizes = createInfo.PoolSizes;
		m_MaxSets = createInfo.MaxSets;
		m_PoolFlags = createInfo.PoolFlags;

		std::vector<VkDescriptorPoolSize> poolSizesVK;
		for (int i = 0; i < m_PoolSizes.size(); i++)
		{
			VH_ASSERT(m_PoolSizes[i], "Incorrectly initialized pool size!");

			VkDescriptorPoolSize poolSizeVK;
			poolSizeVK.descriptorCount = m_PoolSizes[i].DescriptorCount;
			poolSizeVK.type = m_PoolSizes[i].PoolType;

			poolSizesVK.push_back(poolSizeVK);
		}

		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = uint32_t(poolSizesVK.size());
		descriptorPoolInfo.pPoolSizes = poolSizesVK.data();
		descriptorPoolInfo.maxSets = m_MaxSets;
		descriptorPoolInfo.flags = m_PoolFlags;

		return (ResultCode)vkCreateDescriptorPool(m_Device->GetHandle(), &descriptorPoolInfo, nullptr, &m_DescriptorPoolHandles[m_CurrentPool]);
	}

	void DescriptorPool::Move(DescriptorPool&& other) noexcept
	{
		m_Device = other.m_Device;
		m_DescriptorPoolHandles = std::move(other.m_DescriptorPoolHandles);
		m_MaxSets = std::move(other.m_MaxSets);
		m_PoolFlags = std::move(other.m_PoolFlags);
		m_PoolSizes = std::move(other.m_PoolSizes);
		m_CurrentPool = std::move(other.m_CurrentPool);
	}

	void DescriptorPool::Destroy()
	{
		if (m_DescriptorPoolHandles.size() == 0)
			return;

		m_DescriptorPoolHandles.clear();

		for (uint32_t i = 0; i <= m_CurrentPool; i++)
		{
			vkDestroyDescriptorPool(m_Device->GetHandle(), m_DescriptorPoolHandles[i], nullptr);
		}
	}

	DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	DescriptorPool::~DescriptorPool()
	{
		Destroy();
	}

	ResultCode DescriptorPool::AllocateDescriptorSets(const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet* descriptor)
	{
		VH_ASSERT(m_DescriptorPoolHandles.size() > 0, "Pool Not Initialized!");

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPoolHandles[m_CurrentPool];
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		if (vkAllocateDescriptorSets(m_Device->GetHandle(), &allocInfo, descriptor) != VK_SUCCESS)
		{
			// If allocation fails, create a new pool and retry allocation.
			ResultCode res = CreateNewPool();
			if (res != ResultCode::Success)
				return res;

			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_DescriptorPoolHandles[m_CurrentPool];
			allocInfo.pSetLayouts = &descriptorSetLayout;
			allocInfo.descriptorSetCount = 1;

			// Attempt to allocate the descriptor set again.
			res = (ResultCode)vkAllocateDescriptorSets(m_Device->GetHandle(), &allocInfo, descriptor);
			
			// If allocation fails again, return the result code
			if (res != ResultCode::Success)
				return res;
		}

		return ResultCode::Success;
	}

	void DescriptorPool::ResetPool(uint32_t index)
	{
		VH_ASSERT(m_DescriptorPoolHandles.size() > 0, "Pool Not Initialized!");

		vkResetDescriptorPool(m_Device->GetHandle(), m_DescriptorPoolHandles[index], 0);
	}

	ResultCode DescriptorPool::CreateNewPool()
	{
		VH_ASSERT(m_DescriptorPoolHandles.size() > 0, "Pool Not Initialized!");

		m_DescriptorPoolHandles.resize(m_CurrentPool + 1);

		m_CurrentPool++;

		std::vector<VkDescriptorPoolSize> poolSizesVK;
		for (int i = 0; i < m_PoolSizes.size(); i++)
		{
			VH_ASSERT(m_PoolSizes[i], "Incorrectly initialized pool size!");

			VkDescriptorPoolSize poolSizeVK{};
			poolSizeVK.descriptorCount = m_PoolSizes[i].DescriptorCount;
			poolSizeVK.type = m_PoolSizes[i].PoolType;

			poolSizesVK.push_back(poolSizeVK);
		}

		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = uint32_t(poolSizesVK.size());
		descriptorPoolInfo.pPoolSizes = poolSizesVK.data();
		descriptorPoolInfo.maxSets = m_MaxSets;
		descriptorPoolInfo.flags = m_PoolFlags;

		m_DescriptorPoolHandles.push_back(VK_NULL_HANDLE);
		return (ResultCode)vkCreateDescriptorPool(m_Device->GetHandle(), &descriptorPoolInfo, nullptr, &m_DescriptorPoolHandles[m_CurrentPool]);
	}

}