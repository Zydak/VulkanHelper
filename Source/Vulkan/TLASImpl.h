#pragma once
#include "Vulkan/TLAS.h"

#include "DeviceImpl.h"
#include "Vulkan/BLASImpl.h"
#include "Vulkan/CommandBufferImpl.h"

namespace VulkanHelper
{
    class TLAS::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            const Vector<SharedPtr<BLAS::Impl>>& blasList,
            const Vector<uint32_t>& instanceCustomIndices,
            const glm::mat4* transforms,
            const SharedPtr<CommandBuffer::Impl>& commandBuffer
        );

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const TLAS& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static TLAS CreatePublicInterface(const SharedPtr<Impl>& impl) { return TLAS(impl); }

        [[nodiscard]] inline VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
    private:
        static VkTransformMatrixKHR ConvertToVulkanMatrix(const glm::mat4& mat);

        static constexpr size_t MAX_SCRATCH_SIZE = 256 * 1024 * 1024; // 256 MB max size for TLAS scratch buffer

        SharedPtr<Device::Impl> m_Device = nullptr;
        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        SharedPtr<Buffer::Impl> m_Buffer;
        Vector<SharedPtr<BLAS::Impl>> m_BlasList;

        Impl(
            const SharedPtr<Device::Impl>& device,
            VkAccelerationStructureKHR handle,
            SharedPtr<Buffer::Impl> buffer,
            const Vector<SharedPtr<BLAS::Impl>>& blasList
        )
            : m_Device(device)
            , m_Handle(handle)
            , m_Buffer(VulkanHelper::Move(buffer))
            , m_BlasList(blasList.Clone())
        {}
    };
}