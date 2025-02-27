#include "pch.h"

#include "Shader.h"
#include "Logger/Logger.h"


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
		m_ModuleHandle = VK_NULL_HANDLE;
	}

	Shader::Shader(Shader&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	Shader::Shader(const CreateInfo& info)
	{
		m_Device = info.Device;
		m_Type = info.Type;
		m_Defines = info.Defines;
		m_Filepath = info.Filepath;

		if (s_GlobalSession == nullptr)
		{
			slang::createGlobalSession(&s_GlobalSession);
		}

		if (m_DXCUtils == nullptr)
		{
			HRESULT hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_DXCUtils));
			VH_CHECK(hres == 0, "Could not init DXC Utiliy");

			hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_DXCCompiler));
			VH_ASSERT(hres == 0, "Could not create DXC Compiler");
		}
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
		std::vector<uint32_t> data;

		slang::TargetDesc targetDesc{};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = s_GlobalSession->findProfile("spirv_1_4");

		std::string shaderFolder = GetPartBeforeLastSlash(filepath) + '/';
		const char* searchPaths[] = { shaderFolder.c_str() };

		std::vector<slang::CompilerOptionEntry> compilerOptions(2);
		compilerOptions[0].name = slang::CompilerOptionName::Optimization;
		compilerOptions[0].value = slang::CompilerOptionValue{ slang::CompilerOptionValueKind::Int, SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_MAXIMAL };

		slang::SessionDesc sessionDesc{};

		std::vector<slang::PreprocessorMacroDesc> macros(defines.size());

		for (int i = 0; i < defines.size(); i++)
		{
			macros.emplace_back(defines[i].Name.c_str(), defines[i].Value.c_str());
		}

		sessionDesc.preprocessorMacros = macros.data();
		sessionDesc.preprocessorMacroCount = macros.size();

		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;

		sessionDesc.searchPaths = searchPaths;
		sessionDesc.searchPathCount = 1;

		sessionDesc.compilerOptionEntries = compilerOptions.data();
		sessionDesc.compilerOptionEntryCount = (uint32_t)compilerOptions.size();

		Microsoft::WRL::ComPtr<slang::ISession> session;
		s_GlobalSession->createSession(sessionDesc, &session);

		Microsoft::WRL::ComPtr<slang::IBlob> diagnostics;
		Microsoft::WRL::ComPtr<slang::IModule> module = session->loadModule(filepath.c_str(), &diagnostics);

		if (diagnostics)
		{
			std::string message((const char*)diagnostics->getBufferPointer());
			if (message.find(": error") != std::string::npos)
				VH_ERROR(message);
			else
				VH_WARN(message);
		}

		if (module == nullptr)
			return data;

		SlangResult res;

		Microsoft::WRL::ComPtr<slang::IEntryPoint> entryPoint;
		res = module->findEntryPointByName("main", &entryPoint);

		if (res != 0)
			return data;

		slang::IComponentType* components[] = { module.Get(), entryPoint.Get() };
		Microsoft::WRL::ComPtr<slang::IComponentType> program;
		res = session->createCompositeComponentType(components, 2, &program);

		if (res != 0)
			return data;

		Microsoft::WRL::ComPtr<slang::IComponentType> linkedProgram;
		Microsoft::WRL::ComPtr<ISlangBlob> diagnosticBlob;
		res = program->link(&linkedProgram, &diagnosticBlob);

		if (diagnosticBlob)
		{
			std::string message((const char*)diagnosticBlob->getBufferPointer());
			if (message.find(": error") != std::string::npos)
				VH_ERROR(message);
			else
				VH_WARN(message);
		}

		if (res != 0)
			return data;

		int entryPointIndex = 0; // only one entry point
		int targetIndex = 0; // only one target
		Microsoft::WRL::ComPtr<slang::IBlob> kernelBlob;
		Microsoft::WRL::ComPtr<slang::IBlob> diagnostics1;

		res = linkedProgram->getEntryPointCode(entryPointIndex, targetIndex, &kernelBlob, &diagnostics1);

		if (diagnostics1)
		{
			std::string message((const char*)diagnostics1->getBufferPointer());
			if (message.find(": error") != std::string::npos)
				VH_ERROR(message);
			else
				VH_WARN(message);
		}

		if (res != 0)
			return data;

		int codeSize = (int)kernelBlob->getBufferSize();

		data.resize(codeSize / 4);

		memcpy(data.data(), kernelBlob->getBufferPointer(), codeSize);

		return data;
	}

	std::vector<uint32_t> Shader::CompileHlsl(const std::string& filepath, const std::vector<Define>& defines)
	{
		std::vector<uint32_t> data;

		HRESULT hres;

		uint32_t codePage = DXC_CP_ACP;
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceBlob;
		std::wstring wideFilepath = std::wstring(filepath.begin(), filepath.end());
		hres = m_DXCUtils->LoadFile(wideFilepath.c_str(), &codePage, &sourceBlob);
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
		
		hres = m_DXCCompiler->Compile(
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