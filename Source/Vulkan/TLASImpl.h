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

        [[nodiscard]] VHResult Update(const glm::mat4* transforms, uint32_t transformCount, VulkanHelper::CommandBuffer& commandBuffer);

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const TLAS& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static TLAS CreatePublicInterface(const SharedPtr<Impl>& impl) { return TLAS(impl); }

        [[nodiscard]] inline VkAccelerationStructureKHR GetHandle() const { return m_Handle; }
    private:
        static VkTransformMatrixKHR ConvertToVulkanMatrix(const glm::mat4& mat);

        static constexpr size_t MAX_SCRATCH_SIZE = 10 * 1024 * 1024; // 10 MB max size for TLAS scratch buffer

        SharedPtr<Device::Impl> m_Device = nullptr;
        VkAccelerationStructureKHR m_Handle = VK_NULL_HANDLE;
        SharedPtr<Buffer::Impl> m_Buffer;
        Vector<SharedPtr<BLAS::Impl>> m_BlasList;
        VulkanHelper::Vector<VkAccelerationStructureInstanceKHR> m_Instances;
        SharedPtr<Buffer::Impl> m_InstancesBuffer;
        SharedPtr<Buffer::Impl> m_ScratchBuffer;

        Impl(
            const SharedPtr<Device::Impl>& device,
            VkAccelerationStructureKHR handle,
            SharedPtr<Buffer::Impl> buffer,
            const Vector<SharedPtr<BLAS::Impl>>& blasList,
            VulkanHelper::Vector<VkAccelerationStructureInstanceKHR> instances,
            SharedPtr<Buffer::Impl> instancesBuffer,
            SharedPtr<Buffer::Impl> scratchBuffer
        )
            : m_Device(device)
            , m_Handle(handle)
            , m_Buffer(VulkanHelper::Move(buffer))
            , m_BlasList(blasList.Clone())
            , m_Instances(Move(instances))
            , m_InstancesBuffer(VulkanHelper::Move(instancesBuffer))
            , m_ScratchBuffer(VulkanHelper::Move(scratchBuffer))
        {}
    };
}