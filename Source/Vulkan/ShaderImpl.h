#pragma once

#include "Vulkan/Shader.h"

#include "DeviceImpl.h"
#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Shader::Impl
    {
    public:
        static void InitializeSession(const char* shaderSearchPath);
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(Device::Impl* device, const char* filepath, ShaderStages stage);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Shader* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Shader CreatePublicInterface(VulkanHelper::UniquePtr<Impl>&& impl) { return Shader(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline VkShaderModule GetShaderModule() const { return m_Shader; }
        [[nodiscard]] inline VkShaderStageFlagBits GetShaderStage() const { return m_Stage; }
        [[nodiscard]] VkPipelineShaderStageCreateInfo GetShaderStageCreateInfo() const;
    private:
        Device::Impl* m_Device;
        VkShaderModule m_Shader;
        VkShaderStageFlagBits m_Stage;

        Impl(Device::Impl* device, VkShaderModule shader, VkShaderStageFlagBits stage)
            : m_Device(device)
            , m_Shader(shader)
            , m_Stage(stage)
        {}
    };
}