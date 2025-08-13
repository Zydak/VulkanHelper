#include "FenceImpl.h"
#include <vulkan/vulkan.h>

#include "Core/Move.h"
#include "DeviceImpl.h"

namespace VulkanHelper
{
    Expected<SharedPtr<Fence::Impl>, VHResult> Fence::Impl::New(const SharedPtr<Device::Impl>& device, bool startSignaled)
    {
        VH_LOG_INFO("Creating Vulkan Fence Implementation");

        if (device == nullptr)
        {
            VH_LOG_ERROR("Invalid Fence configuration: Device is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = startSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

        VkFence fence;
        VkResult res = vkCreateFence(device->GetDevice(), &fenceInfo, nullptr, &fence);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create fence for swapchain implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return SharedPtr<Impl>(new Impl(device, fence));
    }

    Fence::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device), m_Fence(other.m_Fence)
    {
        other.m_Device = nullptr;
        other.m_Fence = VK_NULL_HANDLE;
    }

    Fence::Impl& Fence::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Cleanup current state

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Fence = other.m_Fence;
        other.m_Fence = VK_NULL_HANDLE;

        return *this;
    }

    Fence::Impl::~Impl()
    {
        if (m_Fence != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan Fence Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Fence);
            m_Fence = VK_NULL_HANDLE;
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
        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.StartSignaled
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Fence::Impl::CreatePublicInterface(implResult.Value());
    }

    Fence::Fence()
        : m_Impl(nullptr)
    {
    }

    Fence::Fence(const Fence& other)
        : m_Impl(other.m_Impl)
    {
    }

    Fence& Fence::operator=(const Fence& other)
    {
        if (this != &other)
        {
            m_Impl = other.m_Impl;
        }
        return *this;
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

    Fence::Fence(const VulkanHelper::SharedPtr<Impl>& impl)
        : m_Impl(impl)
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