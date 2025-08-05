#include "PushConstantImpl.h"
#include "Log/Log.h"
#include "Core/Move.h"

#include <vulkan/vulkan.h>
#include <cstring>

namespace VulkanHelper
{
    Expected<UniquePtr<PushConstant::Impl>, VHResult> PushConstant::Impl::New(const PushConstant::Config& config)
    {
        VH_LOG_INFO("Creating PushConstant Implementation");

        if (config.Stage == ShaderStages::UNDEFINED)
        {
            VH_LOG_ERROR("PushConstant shader stage cannot be UNDEFINED!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.Size == 0)
        {
            VH_LOG_ERROR("PushConstant size cannot be 0!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Vulkan has limits on push constant size, typically 128-256 bytes
        if (config.Size > 128)
        {
            VH_LOG_WARN("PushConstant size {} bytes exceeds common Vulkan limit of 128 bytes. Check device limits. Also note that this may not run on platforms with stricter limits. If you really need more than 128 bytes of space, consider using a Uniform buffer.", config.Size);
        }

        return UniquePtr<Impl>(new Impl(config.Stage, config.Data, config.Size));
    }

    PushConstant::Impl::Impl(ShaderStages stage, const void* data, uint32_t size)
        : m_Stage(stage), m_Data(size)
    {
        if (data != nullptr && size > 0)
        {
            std::memcpy(m_Data.Data(), data, size);
        }
    }

    PushConstant::Impl::Impl(Impl&& other) noexcept
        : m_Stage(other.m_Stage), m_Data(VulkanHelper::Move(other.m_Data))
    {
        other.m_Stage = ShaderStages::UNDEFINED;
    }

    PushConstant::Impl& PushConstant::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Stage = other.m_Stage;
        m_Data = VulkanHelper::Move(other.m_Data);

        other.m_Stage = ShaderStages::UNDEFINED;

        return *this;
    }

    PushConstant::Impl::~Impl()
    {
        VH_LOG_INFO("Destroying PushConstant Implementation");
    }

    VHResult PushConstant::Impl::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        if (data == nullptr)
        {
            VH_LOG_ERROR("Cannot set null data to PushConstant!");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (size == 0)
        {
            VH_LOG_ERROR("Cannot set zero-sized data to PushConstant!");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (offset + size > m_Data.Size())
        {
            VH_LOG_ERROR("PushConstant data exceeds allocated size!");
            return VHResult::WRONG_ARGUMENTS;
        }

        std::memcpy(m_Data.Data() + offset, data, size);

        return VHResult::OK;
    }

    VkPushConstantRange PushConstant::Impl::GetVkPushConstantRange() const
    {
        VkPushConstantRange range{};
        range.stageFlags = static_cast<VkShaderStageFlags>(m_Stage);
        range.offset = 0; // 0 for like 99% of use cases
        range.size = static_cast<uint32_t>(m_Data.Size());
        return range;
    }

    //
    //  Forward functions
    //

    Expected<PushConstant, VHResult> PushConstant::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return Unexpected(implResult.Error());
        }

        return PushConstant{ VulkanHelper::Move(implResult.Value()) };
    }

    PushConstant::PushConstant(UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
    }

    PushConstant::PushConstant(PushConstant&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {
    }

    PushConstant& PushConstant::operator=(PushConstant&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = VulkanHelper::Move(other.m_Impl);
        return *this;
    }

    PushConstant::~PushConstant()
    {
    }

    VHResult PushConstant::SetData(const void* data, uint32_t size, uint32_t offset)
    {
        return m_Impl->SetData(data, size, offset);
    }

    ShaderStages PushConstant::GetStage() const
    {
        return m_Impl->GetStage();
    }

    const void* PushConstant::GetData() const
    {
        return m_Impl->GetData();
    }

    uint32_t PushConstant::GetSize() const
    {
        return m_Impl->GetSize();
    }
}
