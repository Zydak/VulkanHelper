#pragma once

#include "Pch.h"
#include "vulkan/vulkan.h"
#include "Vulkan/ErrorCodes.h"

namespace VulkanHelper
{
	class Device;

	class DescriptorSetLayout
	{
	public:
		struct Binding
		{
			int BindingNumber = -1;
			uint32_t DescriptorsCount = 1;
			VkDescriptorType Type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			VkShaderStageFlags StageFlags = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			operator bool() const
			{
				return (BindingNumber != -1) && (Type != VK_DESCRIPTOR_TYPE_MAX_ENUM) && (StageFlags != VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);
			}
		};

		struct CreateInfo
		{
			Device* Device = nullptr;
			std::vector<Binding> Bindings;
		};
		[[nodiscard]] ResultCode Init(const CreateInfo& createInfo);

		DescriptorSetLayout() = default;
		~DescriptorSetLayout();

		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

		DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
		DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept;

		[[nodiscard]] inline VkDescriptorSetLayout GetDescriptorSetLayoutHandle() const { return m_DescriptorSetLayoutHandle; }
		[[nodiscard]] inline std::vector<Binding> GetDescriptorSetLayoutBindings() { return m_Bindings; }

	private:
		Device* m_Device = nullptr;
		VkDescriptorSetLayout m_DescriptorSetLayoutHandle = VK_NULL_HANDLE;
		std::vector<Binding> m_Bindings;

		void Move(DescriptorSetLayout&& other) noexcept;
		void Destroy();

		friend class DescriptorWriter;
	};
}