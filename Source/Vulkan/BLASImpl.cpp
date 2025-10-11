#include "BLASImpl.h"

#include <vulkan/vulkan.h>

#include "Core/Move.h"
#include "DeviceImpl.h"
#include "BufferImpl.h"
#include "Vulkan/CommandBuffer.h"
#include "CommandBufferImpl.h"
#include "Log/Log.h"

#include "FunctionLoader.h"

namespace VulkanHelper
{
    BLAS::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Handle(other.m_Handle)
        , m_Buffer(Move(other.m_Buffer))
    {
        other.m_Device = nullptr;
        other.m_Handle = VK_NULL_HANDLE;
        other.m_Buffer = nullptr;
    }

    BLAS::Impl& BLAS::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        // Clean up current resources
        if (m_Device && m_Handle != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying BLAS Implementation");

            FunctionLoader::vkDestroyAccelerationStructureKHR(m_Device->GetDevice(), m_Handle, nullptr);
        }

        // Move from other
        m_Device = other.m_Device;
        m_Handle = other.m_Handle;
        m_Buffer = Move(other.m_Buffer);

        // Reset other
        other.m_Device = nullptr;
        other.m_Handle = VK_NULL_HANDLE;
        other.m_Buffer = nullptr;

        return *this;
    }

    BLAS::Impl::~Impl()
    {
        if (m_Device && m_Handle != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying BLAS Implementation");

            FunctionLoader::vkDestroyAccelerationStructureKHR(m_Device->GetDevice(), m_Handle, nullptr);
        }

        m_Handle = VK_NULL_HANDLE;
        m_Device = nullptr;
    }

    VkDeviceAddress BLAS::Impl::GetDeviceAddress() const
    {
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = m_Handle;

        return FunctionLoader::vkGetAccelerationStructureDeviceAddressKHR(m_Device->GetDevice(), &addressInfo);
    }

    //
    //  Forward Functions
    //

    BLAS::BLAS()
        : m_Impl(nullptr)
    {
    }

    BLAS::BLAS(const BLAS& other)
        : m_Impl(other.m_Impl)
    {}

    BLAS& BLAS::operator=(const BLAS& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;
        return *this;
    }

    BLAS::BLAS(BLAS&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {}

    BLAS& BLAS::operator=(BLAS&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = Move(other.m_Impl);
        return *this;
    }

    BLAS::~BLAS()
    {

    }

    BLAS::BLAS(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {}
}
