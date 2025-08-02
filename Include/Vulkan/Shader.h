#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"
#include "Vulkan/Device.h"

namespace VulkanHelper
{
    class Shader
    {
    public:
        struct Config
        {
            Device* device = nullptr;
            const char* Filepath = "";
        };

        static void InitializeSession(const char* shaderSearchPath);
        [[nodiscard]] static Expected<Shader, VHResult> New(const Config& config); 

        Shader(const Shader& other) = delete;
        Shader& operator=(const Shader& other) = delete;
        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;
        
        ~Shader();

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Shader(VulkanHelper::UniquePtr<Impl>&& impl);
    };
}