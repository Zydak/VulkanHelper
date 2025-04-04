#include "pch.h"

#include "Shader.h"
#include "Logger/Logger.h"
#include "Device.h"


namespace VulkanHelper
{

	VulkanHelper::ResultCode Shader::Compile()
	{
		if (!std::filesystem::exists(m_Filepath))
		{
			return ResultCode::FileNotFound;
		}

		std::vector<uint32_t> data = CompileSource(m_Filepath, m_Defines, false);
		if (data.empty())
			return ResultCode::ShaderCompilationFailed;

		VkShaderModuleCreateInfo createInfo{};

		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size() * 4;
		createInfo.pCode = data.data();

		if (vkCreateShaderModule(m_Device->GetHandle(), &createInfo, nullptr, &m_ModuleHandle) != VK_SUCCESS)
		{
			return ResultCode::UnknownError;
		}

		m_Device->SetObjectName(VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)m_ModuleHandle, m_Filepath.c_str());

		return ResultCode::Success;
	}

	void Shader::Destroy()
	{
		if (m_ModuleHandle == VK_NULL_HANDLE)
			return;

		vkDestroyShaderModule(m_Device->GetHandle(), m_ModuleHandle, nullptr);

		Reset();
	}

	Shader::Shader(Shader&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	Shader& Shader::operator=(Shader&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	Shader::~Shader()
	{
		Destroy();
	}

	static std::string GetPartAfterLastSlash(const std::string& str)
	{
		size_t lastSlashPos = str.find_last_of('/');
		if (lastSlashPos != std::string::npos && lastSlashPos != str.length() - 1)
		{
			return str.substr(lastSlashPos + 1);
		}
		return str; // Return the original string if no slash is found or if the last character is a slash
	}

	static std::string GetPartBeforeLastSlash(const std::string& str)
	{
		size_t lastSlashPos = str.find_last_of('/');
		if (lastSlashPos != std::string::npos && lastSlashPos != str.length() - 1)
		{
			return str.substr(0, lastSlashPos);
		}
		return str; // Return the original string if no slash is found or if the last character is a slash
	}

	ResultCode Shader::Init(const CreateInfo& createInfo)
	{
		Destroy();

		m_Device = createInfo.Device;
		m_Type = createInfo.Type;
		m_Defines = createInfo.Defines;
		m_Filepath = createInfo.Filepath;

		if (s_DXCUtils == nullptr)
		{
			HRESULT hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&s_DXCUtils));
			VH_CHECK(hres == 0, "Could not init DXC Utiliy");

			hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&s_DXCCompiler));
			VH_ASSERT(hres == 0, "Could not create DXC Compiler");
		}

		return Compile();
	}

	std::vector<uint32_t> Shader::CompileSource(const std::string& filepath, const std::vector<Define>& defines, bool cacheToFile)
	{
		size_t dotPos = filepath.find_last_of('.');
		std::string extension = filepath.substr(dotPos, filepath.size() - dotPos);

		if (extension == ".slang")
		{
			return CompileSlang(filepath, defines);
		}
		else if (extension == ".hlsl")
		{
			return CompileHlsl(filepath, defines);
		}
		else
		{
			VH_ERROR("Unsupported shader extension: {0}", extension);
			return {};
		}
	}

	std::vector<uint32_t> Shader::CompileSlang(const std::string& filepath, const std::vector<Define>& defines)
	{
		return {};
	}

	std::vector<uint32_t> Shader::CompileHlsl(const std::string& filepath, const std::vector<Define>& defines)
	{
		std::vector<uint32_t> data;

		HRESULT hres;

		uint32_t codePage = DXC_CP_ACP;
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
		std::wstring wideFilepath = std::wstring(filepath.begin(), filepath.end());
		hres = s_DXCUtils->LoadFile(wideFilepath.c_str(), &codePage, &sourceBlob);
		VH_ASSERT(hres == 0, "Could not load file");

		// Select target profile based on shader file extension
		LPCWSTR targetProfile{};
		switch (m_Type)
		{
		case VK_SHADER_STAGE_VERTEX_BIT:
			targetProfile = L"vs_6_4";
			break;
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			targetProfile = L"ps_6_4";
			break;
		case VK_SHADER_STAGE_COMPUTE_BIT:
			targetProfile = L"cs_6_4";
			break;
		default:
			VH_ASSERT(false, "Unsupported shader type");
			break;
		}

		// Configure the compiler arguments for compiling the HLSL shader to SPIR-V
		std::vector<LPCWSTR> arguments = {
			// (Optional) name of the shader file to be displayed e.g. in an error message
			wideFilepath.c_str(),
			// Shader main entry point
			L"-E", L"main",
			// Enable Optimizations
			L"-O3",
			// Shader target profile
			L"-T", targetProfile,
			// Compile to SPIRV
			L"-spirv"
		};

		// Compile shader
		DxcBuffer buffer{};
		buffer.Encoding = DXC_CP_ACP;
		buffer.Ptr = sourceBlob->GetBufferPointer();
		buffer.Size = sourceBlob->GetBufferSize();

		Microsoft::WRL::ComPtr<IDxcResult> result{ nullptr };
		
		hres = s_DXCCompiler->Compile(
			&buffer,
			arguments.data(),
			(uint32_t)arguments.size(),
			nullptr,
			IID_PPV_ARGS(&result)
		);

		if (SUCCEEDED(hres)) 
		{
			result->GetStatus(&hres);
		}

		// Output error if compilation failed
		if (FAILED(hres) && (result)) 
		{
			Microsoft::WRL::ComPtr<IDxcBlobEncoding> errorBlob;
			hres = result->GetErrorBuffer(&errorBlob);
			if (SUCCEEDED(hres) && errorBlob) 
			{
				VH_ERROR("Shader compilation failed :\n\n{0}", (const char*)errorBlob->GetBufferPointer());
				return data;
			}
		}

		Microsoft::WRL::ComPtr<IDxcBlob> code;
		result->GetResult(&code);

		int codeSize = (int)code->GetBufferSize();

		data.resize(codeSize / 4);

		memcpy(data.data(), code->GetBufferPointer(), codeSize);

		return data;
	}

	void Shader::Move(Shader&& other) noexcept
	{
		m_Device = other.m_Device;
		other.m_Device = nullptr;

		m_ModuleHandle = other.m_ModuleHandle;
		other.m_ModuleHandle = VK_NULL_HANDLE;

		m_Type = other.m_Type;
		other.m_Type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

		m_Defines = std::move(other.m_Defines);
		other.m_Defines.clear();

		m_Filepath = std::move(other.m_Filepath);
		other.m_Filepath = "";

		other.Reset();
	}

	void Shader::Reset()
	{
		m_Device = nullptr;
		m_ModuleHandle = VK_NULL_HANDLE;
		m_Type = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		m_Defines.clear();
		m_Filepath = "";
	}

	VkPipelineShaderStageCreateInfo Shader::GetStageCreateInfo()
	{
		VkPipelineShaderStageCreateInfo stage;
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.stage = m_Type;
		stage.module = m_ModuleHandle;
		stage.pName = "main";
		stage.flags = 0;
		stage.pNext = nullptr;
		stage.pSpecializationInfo = nullptr;

		return stage;
	}

}