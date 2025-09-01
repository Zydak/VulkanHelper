#include "SemaphoreImpl.h"
#include "DeviceImpl.h"


#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    Expected<SharedPtr<Semaphore::Impl>, VHResult> Semaphore::Impl::New(const SharedPtr<Device::Impl>& device)
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoreInfo.flags = 0; // Semaphores don't support initial signaled state

        VkSemaphore semaphore;
        VkResult res = vkCreateSemaphore(device->GetDevice(), &semaphoreInfo, nullptr, &semaphore);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create semaphore");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(device, semaphore)));
    }

    Semaphore::Impl::Impl(Semaphore::Impl&& other) noexcept
        : m_Device(other.m_Device), m_Semaphore(other.m_Semaphore)
    {
        other.m_Device = nullptr;
        other.m_Semaphore = VK_NULL_HANDLE;
    }

    Semaphore::Impl& Semaphore::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Semaphore = other.m_Semaphore;
        other.m_Semaphore = VK_NULL_HANDLE;

        return *this;
    }

    Semaphore::Impl::~Impl()
    {
        if (m_Semaphore != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan Semaphore Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Semaphore);
            m_Semaphore = VK_NULL_HANDLE;
            m_Device = nullptr;
        }
    }

    //
    //  Forward Functions
    //

    VulkanHelper::Expected<Semaphore, VHResult> Semaphore::New(const Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Semaphore Implementation");

        auto implResult = Impl::New(Device::Impl::GetImplementation(config.Device));
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Semaphore::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    Semaphore::Semaphore()
        : m_Impl(nullptr)
    {
    }

    Semaphore::Semaphore(const Semaphore& other)
        : m_Impl(other.m_Impl)
    {
    }

    Semaphore::~Semaphore()
    {

    }

    Semaphore::Semaphore(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }

    Semaphore::Semaphore(Semaphore&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Semaphore& Semaphore::operator=(const Semaphore& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;

        return *this;
    }
}