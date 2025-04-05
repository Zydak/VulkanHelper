#pragma once
#include "Pch.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
	template<typename T>
	class PushConstant
	{
	public:
		struct CreateInfo
		{
			VkShaderStageFlags Stage;
		};
		
		void Init(CreateInfo createInfo);

		PushConstant() = default;
		PushConstant(CreateInfo createInfo);
		~PushConstant();

		PushConstant(const PushConstant& other) = delete;
		PushConstant& operator=(const PushConstant& other) = delete;
		PushConstant(PushConstant&& other) noexcept;
		PushConstant& operator=(PushConstant&& other) noexcept;

		void SetData(const T& data);
		void Push(VkPipelineLayout Layout, VkCommandBuffer cmdBuffer, uint32_t offset = 0);

		inline VkPushConstantRange GetRange() const { return m_Range; }
		inline VkPushConstantRange* GetRangePtr() { return &m_Range; }
		inline T GetData() const { return m_Data; }
		inline T* GetDataPtr() { return &m_Data; }
	private:
		T m_Data = T{};

		VkPushConstantRange m_Range = {};
		VkShaderStageFlags m_Stage = {};

		void Destroy();
		void Reset();
	};

	template<typename T>
	void VulkanHelper::PushConstant<T>::Reset()
	{
		m_Data = T{};
		m_Range = {};
		m_Stage = 0;
	}

	template<typename T>
	PushConstant<T>& PushConstant<T>::operator=(PushConstant<T>&& other) noexcept
	{
		m_Data = std::move(other.m_Data);
		m_Range = std::move(other.m_Range);
		m_Stage = std::move(other.m_Stage);

		other.Reset();

		return *this;
	}

	template<typename T>
	PushConstant<T>::PushConstant(PushConstant&& other) noexcept
	{
		m_Data = std::move(other.m_Data);
		m_Range = std::move(other.m_Range);
		m_Stage = std::move(other.m_Stage);

		other.Reset();
	}

	template<typename T>
	void VulkanHelper::PushConstant<T>::SetData(const T& data)
	{
		m_Data = data;
	}

	template<typename T>
	void VulkanHelper::PushConstant<T>::Destroy()
	{
		Reset();
	}

	template<typename T>
	void VulkanHelper::PushConstant<T>::Init(CreateInfo createInfo)
	{
		Destroy();

		m_Stage = createInfo.Stage;

		m_Range.offset = 0;
		m_Range.size = sizeof(T);
		m_Range.stageFlags = m_Stage;
	}

	template<typename T>
	void VulkanHelper::PushConstant<T>::Push(VkPipelineLayout Layout, VkCommandBuffer cmdBuffer, uint32_t offset /*= 0*/)
	{
		vkCmdPushConstants(
			cmdBuffer,
			Layout,
			m_Stage,
			offset,
			sizeof(T),
			&m_Data
		);
	}

	template<typename T>
	VulkanHelper::PushConstant<T>::~PushConstant()
	{
		Destroy();
	}

	template<typename T>
	VulkanHelper::PushConstant<T>::PushConstant(CreateInfo createInfo)
	{
		Init(createInfo);
	}

}