#pragma once

#include "Vulkan/BLAS.h"
#include "DeviceImpl.h"
#include "VulkanMemoryAllocator.h"
#include "Core/Vector.h"
#include "BufferImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class BLAS::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            const Vector<SharedPtr<Buffer::Impl>>& vertexBuffers,
            uint32_t vertexSize,
            const Vector<SharedPtr<Buffer::Impl>>& indexBuffers,
            bool enableCompaction,
            const SharedPtr<CommandBuffer::Impl>& commandBuffer
        );

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
        VHResult Build(
            const SharedPtr<CommandBuffer::Impl>& commandBuffer,
            VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos,
            VkAccelerationStructureBuildGeometryInfoKHR& buildInfo
        );
        VHResult Compact(const SharedPtr<CommandBuffer::Impl>& commandBuffer);

        static constexpr size_t BLAS_MAX_SIZE = 256 * 1024 * 1024; // 256 MB max size for BLAS

        SharedPtr<Device::Impl> m_Device = nullptr;
        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        SharedPtr<Buffer::Impl> m_Buffer;
        uint64_t m_ScratchBufferSize = 0;

        Impl(
            const SharedPtr<Device::Impl>& device,
            VkAccelerationStructureKHR handle, 
            const SharedPtr<Buffer::Impl>& buffer,
            uint64_t scratchBufferSize
        )
            : m_Device(device)
            , m_Handle(handle)
            , m_Buffer(buffer)
            , m_ScratchBufferSize(scratchBufferSize)
        {}
        
    };
} 