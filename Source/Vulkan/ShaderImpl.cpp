#include "ShaderImpl.h"

#include <array>
#include <filesystem>
#include <vector>

#include "slang.h"
#include "slang-com-ptr.h"
#include "slang-com-helper.h"
#include <vulkan/vulkan.h>

#include "DeviceImpl.h"

#include <fstream>

static inline Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;
static inline Slang::ComPtr<slang::ISession> s_Session;

namespace VulkanHelper
{
    void Shader::Impl::InitializeSession(const char* shaderSearchPath)
    {
        slang::createGlobalSession(s_GlobalSession.writeRef());

        static slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.profile = s_GlobalSession->findProfile("spirv_1_5");

        std::vector<slang::CompilerOptionEntry> compilerOptions(1);
		compilerOptions[0].name = slang::CompilerOptionName::Optimization;
		compilerOptions[0].value = slang::CompilerOptionValue{ slang::CompilerOptionValueKind::Int, SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_MAXIMAL };

        static std::string searchPath = std::string(std::filesystem::current_path().c_str()) + "/../../" + shaderSearchPath;
        const char* searchPathCStr = searchPath.c_str();
        VH_LOG_DEBUG(searchPath.c_str());
        static slang::SessionDesc sessionDesc = {};
        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;
        sessionDesc.searchPaths = &searchPathCStr;
        sessionDesc.searchPathCount = 1;
		sessionDesc.compilerOptionEntries = compilerOptions.data();
		sessionDesc.compilerOptionEntryCount = (uint32_t)compilerOptions.size();

        s_GlobalSession->createSession(sessionDesc, s_Session.writeRef());
    }

    VulkanHelper::Expected<VulkanHelper::UniquePtr<Shader::Impl>, VHResult> Shader::Impl::New(const Config& config)
    {
        (void)config;
        std::string filepath = config.Filepath;
        if (filepath == "")
        {
            VH_LOG_ERROR("Shader Filepath cannot be empty!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        Slang::ComPtr<slang::IModule> slangModule;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            std::string moduleName = filepath.substr(0, filepath.find(".slang"));
            slangModule = s_Session->loadModule(moduleName.c_str(), diagnosticsBlob.writeRef());
            if (diagnosticsBlob != nullptr)
            {
                VH_LOG_ERROR((const char*)diagnosticsBlob->getBufferPointer());
            }
            if (!slangModule)
            {
                return Unexpected(VHResult::INITIALIZATION_FAILED);
            }
        }

        Slang::ComPtr<slang::IEntryPoint> entryPoint;
        {
            Slang::ComPtr<slang::IBlob> diagnosticsBlob;
            SlangResult result = slangModule->findEntryPointByName("Main", entryPoint.writeRef());
            
            if (result < 0)
                return Unexpected(VHResult::INITIALIZATION_FAILED);
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
                return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        std::vector<uint32_t> data;
        size_t codeSize = spirvCode->getBufferSize();
        data.resize(codeSize / 4);
        memcpy(data.data(), spirvCode->getBufferPointer(), codeSize);
        VH_LOG_DEBUG((const char*)spirvCode->getBufferPointer());

        VkShaderModuleCreateInfo createInfo{};

		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = data.size() * 4;
		createInfo.pCode = data.data();

        VkShaderModule module;

        vkCreateShaderModule(Device::Impl::GetImplementation(config.device)->GetDevice(), &createInfo, nullptr, &module);

        return Unexpected(VHResult::NOT_IMPLEMENTED);
    }

    Shader::Impl::~Impl()
    {

    }

    Shader::Impl::Impl(Impl&& other) noexcept
    {
        (void) other;
    }

    Shader::Impl& Shader::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;
        
        return *this;
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
        auto implResult = Impl::New(config);
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