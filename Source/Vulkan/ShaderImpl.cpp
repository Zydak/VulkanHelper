#include "ShaderImpl.h"

#include <array>
#include <filesystem>
#include <vector>

#include <fstream>

#include "slang.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"

namespace VulkanHelper
{
    // It's here instead of as a member variable so I don't have to include slang in the header file
    static inline Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;
    static inline Slang::ComPtr<slang::ISession> s_Session;

    void Shader::Impl::InitializeSession(const char* shaderSearchPath)
    {
        slang::createGlobalSession(s_GlobalSession.writeRef());

        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = s_GlobalSession->findProfile("spirv_1_5");

        std::vector<slang::CompilerOptionEntry> compilerOptions(1);
		compilerOptions[0].name = slang::CompilerOptionName::Optimization;
		compilerOptions[0].value = slang::CompilerOptionValue{ slang::CompilerOptionValueKind::Int, SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_MAXIMAL };

        std::string searchPath = std::filesystem::current_path().string() + "/" + shaderSearchPath;
        const char* searchPathCStr = searchPath.c_str();
        VH_LOG_DEBUG("slang session search path: {}", searchPath.c_str());
        slang::SessionDesc sessionDesc = {};
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;
        sessionDesc.searchPaths = &searchPathCStr;
        sessionDesc.searchPathCount = 1;
		sessionDesc.compilerOptionEntries = compilerOptions.data();
		sessionDesc.compilerOptionEntryCount = (uint32_t)compilerOptions.size();

        s_GlobalSession->createSession(sessionDesc, s_Session.writeRef());
    }

    VulkanHelper::Expected<VulkanHelper::UniquePtr<Shader::Impl>, VHResult> Shader::Impl::New(Device::Impl* device, const char* filepath, ShaderStages stage)
    {
        if (stage == ShaderStages::UNDEFINED)
        {
            VH_LOG_ERROR("Shader Stage Can't Be UNDEFINED!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        std::string filepathStr = filepath;
        if (filepathStr == "")
        {
            VH_LOG_ERROR("Shader Filepath cannot be empty!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        Slang::ComPtr<slang::IModule> slangModule;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            std::string moduleName = filepathStr.substr(0, filepathStr.find(".slang"));
            slangModule = s_Session->loadModule(moduleName.c_str(), diagnosticsBlob.writeRef());
            if (diagnosticsBlob != nullptr)
            {
                VH_LOG_ERROR("{}", (const char*)diagnosticsBlob->getBufferPointer());
            }
            if (!slangModule)
            {
                return Unexpected(VHResult::NO_SPECIFIED_SHADER_MODULE_FOUND);
            }
        }

        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        {
            SlangResult result = slangModule->findEntryPointByName("Main", entryPoint.writeRef());
            
            if (result < 0)
            {
                VH_LOG_ERROR("No entry point named 'Main' found in shader module '{}', make sure it is name Main and not main.", filepathStr);
                return Unexpected(VHResult::NO_SPECIFIED_ENTRY_POINT_FOUND);
            }
        }

        std::array<slang::IComponentType*, 2> componentTypes =
        {
            slangModule,
            entryPoint
        };

        Slang::ComPtr<slang::IComponentType> composedProgram;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = s_Session->createCompositeComponentType(
                componentTypes.data(),
                componentTypes.size(),
                composedProgram.writeRef(),
                diagnosticsBlob.writeRef()
            );

            if (diagnosticsBlob != nullptr)
                VH_LOG_ERROR((const char*)diagnosticsBlob->getBufferPointer());
            if (result < 0)
                return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = composedProgram->link(
                linkedProgram.writeRef(),
                diagnosticsBlob.writeRef()
            );

            if (diagnosticsBlob != nullptr)
                VH_LOG_ERROR((const char*)diagnosticsBlob->getBufferPointer());
            if (result < 0)
                return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        Slang::ComPtr<slang::IBlob> spirvCode;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = linkedProgram->getEntryPointCode(
                0, // entryPointIndex
                0, // targetIndex
                spirvCode.writeRef(),
                diagnosticsBlob.writeRef()
            );

            if (diagnosticsBlob != nullptr)
                VH_LOG_ERROR((const char*)diagnosticsBlob->getBufferPointer());
            if (result < 0)
                return Unexpected(VHResult::SHADER_COMPILATION_FAILED);
        }

        std::vector<uint32_t> data;
        size_t codeSize = spirvCode->getBufferSize();
        data.resize(codeSize / 4);
        memcpy(data.data(), spirvCode->getBufferPointer(), codeSize);

        VkShaderModuleCreateInfo createInfo{};

		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size() * 4;
		createInfo.pCode = data.data();

        VkShaderModule module;

        VkResult res = vkCreateShaderModule(device->GetDevice(), &createInfo, nullptr, &module);
        if (res != VK_SUCCESS)
            return Unexpected(VHResult(res));

        return UniquePtr(new Impl(device, module, (VkShaderStageFlagBits)stage));
    }

    Shader::Impl::~Impl()
    {
        if (m_Shader != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Shader Module Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Shader);
            m_Shader = VK_NULL_HANDLE;
            m_Device = nullptr;
        }
    }

    Shader::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device), m_Shader(other.m_Shader), m_Stage(other.m_Stage)
    {
        other.m_Device = nullptr;
        other.m_Shader = VK_NULL_HANDLE;
    }

    Shader::Impl& Shader::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Cleanup current state

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Shader = other.m_Shader;
        other.m_Shader = VK_NULL_HANDLE;
        m_Stage = other.m_Stage;
        
        return *this;
    }

    VkPipelineShaderStageCreateInfo Shader::Impl::GetShaderStageCreateInfo() const
    {
        VkPipelineShaderStageCreateInfo stage;
		stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage.stage = m_Stage;
		stage.module = m_Shader;
		stage.pName = "main";
		stage.flags = 0;
		stage.pNext = nullptr;
		stage.pSpecializationInfo = nullptr;

		return stage;
    }

    //
    // Forward functions
    //

    void Shader::InitializeSession(const char* shaderSearchPath)
    {
        Shader::Impl::InitializeSession(shaderSearchPath);
    }
    
    VulkanHelper::Expected<Shader, VHResult> Shader::New(const Config& config)
    {
        VH_LOG_INFO("Creating Shader Module Implementation");

        if (config.device == nullptr)
        {
            VH_LOG_ERROR("Device Can't Be NULL!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        auto implResult = Impl::New(Device::Impl::GetImplementation(config.device), config.Filepath, config.Stage);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Shader{ VulkanHelper::Move(implResult.Value()) };
    }

    Shader::~Shader()
    {

    }

    Shader::Shader(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    Shader::Shader(Shader&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Shader(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }
}