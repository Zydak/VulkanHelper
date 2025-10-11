#pragma once

#include "Vulkan/BLAS.h"
#include "DeviceImpl.h"
#include "Core/Vector.h"
#include "BufferImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class BLASBuilder;

    class BLAS::Impl
    {
    public:
        // Created only by BLASBuilder
        
        ~Impl();

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const BLAS& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static BLAS CreatePublicInterface(const SharedPtr<Impl> impl) { return BLAS(impl); }

        [[nodiscard]] inline VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
        [[nodiscard]] VkDeviceAddress GetDeviceAddress() const;

    private:

        SharedPtr<Device::Impl> m_Device = nullptr;
        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        SharedPtr<Buffer::Impl> m_Buffer;

        Impl(
            const SharedPtr<Device::Impl>& device,
            VkAccelerationStructureKHR handle, 
            const SharedPtr<Buffer::Impl>& buffer
        )
            : m_Device(device)
            , m_Handle(handle)
            , m_Buffer(buffer)
        {}

        friend class BLASBuilder;
    };
} 