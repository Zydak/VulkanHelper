#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Vulkan/BLAS.h"
#include "Vulkan/BLASBuilder.h"
#include "DeviceImpl.h"
#include "Vulkan/Buffer.h"
#include "Core/Vector.h"
#include "BufferImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class BLASBuilder::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(const SharedPtr<Device::Impl>& device);

        ~Impl();

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const BLASBuilder& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static BLASBuilder CreatePublicInterface(const SharedPtr<Impl> impl) { return BLASBuilder(impl); }

        [[nodiscard]] Expected<Vector<BLAS>, VHResult> Build(BLAS::Config* blasConfigs, uint32_t count, CommandBuffer& computeCmd);

        VHResult Compact(Vector<BLAS>& blasList, CommandBuffer& commandBuffer);

    private:
        struct BLASBuildData
        {
            Vector<VkAccelerationStructureGeometryKHR> geometries;
            Vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
            Vector<uint32_t> maxPrimitiveCounts;
            Vector<VkAccelerationStructureBuildSizesInfoKHR> buildSizesInfos;
            Vector<VkAccelerationStructureKHR> accelerationStructures;
            Vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometryInfos;
            Vector<SharedPtr<Buffer::Impl>> asBuffers;
        };

        Expected<Vector<BLASBuildData>, VHResult> Prepare(BLAS::Config* blasConfigs, uint32_t count, CommandBuffer& commandBuffer);

        Expected<Vector<BLAS>, VHResult> Build(
            const Vector<BLASBuildData>& buildDataList,
            CommandBuffer& commandBuffer
        );

        SharedPtr<Device::Impl> m_Device = nullptr;
        SharedPtr<Buffer::Impl> m_ScratchBuffer;

        static const uint64_t SCRATCH_BUFFER_SIZE = 10 * 1024 * 1024; // 10 MB

        Impl(
            const SharedPtr<Device::Impl>& device,
            const SharedPtr<Buffer::Impl>& scratchBuffer
        )
            : m_Device(device)
            , m_ScratchBuffer(scratchBuffer)
        {}
        
    };
} 