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
        [[nodiscard]] static Expected<BLAS, VHResult> New(
            Device::Impl* device,
            const Vector<Buffer::Impl*>& vertexBuffers,
            uint32_t vertexSize,
            const Vector<Buffer::Impl*>& indexBuffers,
            bool enableCompaction,
            CommandBuffer::Impl* commandBuffer
        );

        ~Impl();

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        [[nodiscard]] inline static Impl* GetImplementation(const BLAS* const publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static BLAS CreatePublicInterface(Impl&& impl) { return BLAS(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<BLAS> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new BLAS(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

        [[nodiscard]] inline VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
        [[nodiscard]] VkDeviceAddress GetDeviceAddress() const;

    private:
        VHResult Build(
            CommandBuffer::Impl* commandBuffer,
            VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos,
            VkAccelerationStructureBuildGeometryInfoKHR& buildInfo
        );
        VHResult Compact(CommandBuffer::Impl* commandBuffer);

        static constexpr size_t BLAS_MAX_SIZE = 256 * 1024 * 1024; // 256 MB max size for BLAS

        Device::Impl* m_Device = nullptr;
        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        Buffer::Impl m_Buffer;
        uint64_t m_ScratchBufferSize = 0;

        Impl(Device::Impl* device, VkAccelerationStructureKHR handle, 
             Buffer::Impl&& buffer,
             uint64_t scratchBufferSize
        )
            : m_Device(device)
            , m_Handle(handle)
            , m_Buffer(Move(buffer))
            , m_ScratchBufferSize(scratchBufferSize)
        {}
        
    };
} 