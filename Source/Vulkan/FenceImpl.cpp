#include "FenceImpl.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Core/Move.h"
#include "DeviceImpl.h"

namespace VulkanHelper
{
    Expected<UniquePtr<Fence::Impl>, VHResult> Fence::Impl::New(const Config& config)
    {
        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Invalid Fence configuration: Device is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VkFence fence;
        VkResult res = vkCreateFence(config.Device->m_Impl->GetDevice(), &fenceInfo, nullptr, &fence);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create fence for swapchain implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return UniquePtr(new Impl(config.Device->m_Impl.Get(), fence));
    }

    Fence::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device), m_Fence(other.m_Fence)
    {
        other.m_Device = nullptr;
        other.m_Fence = nullptr;
    }

    Fence::Impl& Fence::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Cleanup current state

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Fence = other.m_Fence;
        other.m_Fence = nullptr;

        return *this;
    }

    Fence::Impl::~Impl()
    {
        if (m_Fence != nullptr)
        {
            vkDestroyFence(m_Device->GetDevice(), m_Fence , nullptr);
            m_Fence = nullptr;
            m_Device = nullptr;
        }
    }

    void Fence::Impl::Wait()
    {
        vkWaitForFences(m_Device->GetDevice(), 1, &m_Fence, VK_TRUE, UINT64_MAX);
    }

    void Fence::Impl::Reset()
    {
        vkResetFences(m_Device->GetDevice(), 1, &m_Fence);
    }

    //
    //  Forward functions
    //

    Expected<Fence, VHResult> Fence::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Fence{ VulkanHelper::Move(implResult.Value()) };
    }

    Fence::Fence(Fence&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {

    }

    Fence& Fence::operator=(Fence&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Fence(); // cleanup current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Fence::~Fence()
    {

    }

    void Fence::Wait()
    {
        m_Impl->Wait();
    }

    void Fence::Reset()
    {
        m_Impl->Reset();
    }
}