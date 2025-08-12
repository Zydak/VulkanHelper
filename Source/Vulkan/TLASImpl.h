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
        [[nodiscard]] static Expected<Impl, VHResult> New(
            Device::Impl* device,
            const Vector<const BLAS::Impl*>& blasList,
            const glm::mat4* transforms,
            CommandBuffer::Impl* commandBuffer
        );

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const TLAS* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static TLAS CreatePublicInterface(UniquePtr<Impl>&& impl) { return TLAS(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
    private:
        static VkTransformMatrixKHR ConvertToVulkanMatrix(const glm::mat4& mat);

        static constexpr size_t MAX_SCRATCH_SIZE = 256 * 1024 * 1024; // 256 MB max size for TLAS scratch buffer

        Device::Impl* m_Device = nullptr;
        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        Buffer::Impl m_Buffer;

        Impl(
            Device::Impl* device,
            VkAccelerationStructureKHR handle,
            Buffer::Impl&& buffer
        )
            : m_Device(device)
            , m_Handle(handle)
            , m_Buffer(VulkanHelper::Move(buffer))
        {}
    };
}