#include "SemaphoreImpl.h"
#include "DeviceImpl.h"


#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    Expected<UniquePtr<Semaphore::Impl>, VHResult> Semaphore::Impl::New(Device::Impl* device)
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

        return UniquePtr(new Impl(device, semaphore));
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
            VH_LOG_INFO("Destroying Vulkan Semaphore Implementation");
            vkDestroySemaphore(m_Device->GetDevice(), m_Semaphore , nullptr);
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

        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Semaphore initialization failed. Device can't be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        auto implResult = Impl::New(Device::Impl::GetImplementation(config.Device));
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Semaphore{ VulkanHelper::Move(implResult.Value()) };
    }

    Semaphore::~Semaphore()
    {

    }

    Semaphore::Semaphore(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    Semaphore::Semaphore(Semaphore&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Semaphore& Semaphore::operator=(Semaphore&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Semaphore(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }
}