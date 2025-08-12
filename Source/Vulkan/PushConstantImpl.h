#pragma once

#include "Vulkan/PushConstant.h"
#include "Core/Enums.h"
#include "Core/Vector.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class PushConstant::Impl
    {
    public:
        [[nodiscard]] static Expected<Impl, VHResult> New(ShaderStages stage, void* data, uint32_t size);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const PushConstant* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static PushConstant CreatePublicInterface(Impl&& impl) { return PushConstant(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<PushConstant> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new PushConstant(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        [[nodiscard]] VHResult SetData(const void* data, uint32_t size, uint32_t offset = 0);

        [[nodiscard]] inline ShaderStages GetStage() const { return m_Stage; }
        [[nodiscard]] inline const void* GetData() const { return m_Data.Data(); }
        [[nodiscard]] inline uint32_t GetSize() const { return static_cast<uint32_t>(m_Data.Size()); }
        [[nodiscard]] VkPushConstantRange GetVkPushConstantRange() const;

    private:
        Impl(ShaderStages stage, const void* data, uint32_t size);

        ShaderStages m_Stage;
        VulkanHelper::Vector<uint8_t> m_Data;
    };
}
