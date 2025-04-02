#pragma once

#define NOMINMAX

#include "pch.h"
#include "vulkan/vulkan_core.h"
#include "ErrorCodes.h"
#include "wrl/client.h"
#include "dxcapi.h"

namespace VulkanHelper
{
	class Device;
	class Shader
	{
	public:
		struct Define
		{
			std::string Name = "";
			std::string Value = "";
		};

		struct CreateInfo
		{
			Device* Device = nullptr;
			std::string Filepath = "";
			VkShaderStageFlagBits Type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

			std::vector<Define> Defines;

			bool CacheToFile = false;
		};

		[[nodiscard]] ResultCode Init(const CreateInfo& createInfo);
		Shader() = default;
		~Shader();

		Shader(const Shader& other) = delete;
		Shader& operator=(const Shader& other) = delete;
		Shader(Shader&& other) noexcept;
		Shader& operator=(Shader&& other) noexcept;

	public:

		[[nodiscard]] ResultCode Compile();

	public:

		[[nodiscard]] VkPipelineShaderStageCreateInfo GetStageCreateInfo();

		[[nodiscard]] inline VkShaderModule GetModuleHandle() { return m_ModuleHandle; }
		[[nodiscard]] inline VkShaderStageFlagBits GetType() const { return m_Type; }

	private:

		[[nodiscard]] std::vector<uint32_t> CompileSource(const std::string& filepath, const std::vector<Define>& defines, bool cacheToFile);
		
		[[nodiscard]] std::vector<uint32_t> CompileSlang(const std::string& filepath, const std::vector<Define>& defines);
		[[nodiscard]] std::vector<uint32_t> CompileHlsl(const std::string& filepath, const std::vector<Define>& defines);

		Device* m_Device = nullptr;
		VkShaderModule m_ModuleHandle = VK_NULL_HANDLE;
		VkShaderStageFlagBits m_Type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		std::vector<Define> m_Defines;
		std::string m_Filepath = "";

		inline static Microsoft::WRL::ComPtr<IDxcUtils> s_DXCUtils = nullptr;
		inline static Microsoft::WRL::ComPtr<IDxcCompiler3> s_DXCCompiler = nullptr;

		void Destroy();
		void Move(Shader&& other) noexcept;
	};
}