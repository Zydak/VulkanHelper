#pragma once

#include "Vulkan/Shader.h"

namespace VulkanHelper
{
    class Shader::Impl
    {
    public:
        static void InitializeSession(const char* shaderSearchPath);
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Config& config);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Shader* publicInterface) { return publicInterface->m_Impl.Get(); }
    private:
    };
}