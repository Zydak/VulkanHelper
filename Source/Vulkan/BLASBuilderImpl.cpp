#include "BLASBuilderImpl.h"

#include "CommandPoolImpl.h"
#include "Core/Enums.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Vulkan/BLAS.h"
#include "Vulkan/BLASBuilder.h"
#include "Vulkan/CommandBuffer.h"

#include "FunctionLoader.h"
#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <sys/types.h>

#include "BLASImpl.h"

namespace VulkanHelper
{
    Expected<SharedPtr<BLASBuilder::Impl>, VHResult> BLASBuilder::Impl::New(const SharedPtr<Device::Impl>& device)
    {
        auto scratchBufferRes = Buffer::Impl::New(
            device,
            SCRATCH_BUFFER_SIZE,
            Buffer::Usage::STORAGE_BUFFER_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not mappable
            device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment,
            "BLASBuilder Scratch Buffer"
        );

        if (!scratchBufferRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create scratch buffer for BLASBuilder");
            return Unexpected(scratchBufferRes.Error());
        }

        auto scratchBuffer = scratchBufferRes.Value();

        auto impl = MakeShared<Impl>(Impl(
            device,
            scratchBuffer
        ));

        return impl;
    }

    Expected<Vector<BLAS>, VHResult> BLASBuilder::Impl::Build(BLAS::Config* blasConfigs, uint32_t count, CommandBuffer& computeCmd)
    {
        auto buildDataListRes = Prepare(blasConfigs, count, computeCmd);

        if (!buildDataListRes.HasValue())
        {
            VH_LOG_ERROR("Failed to prepare BLAS build data");
            return Unexpected(buildDataListRes.Error());
        }

        return Build(buildDataListRes.Value(), computeCmd);
    }

    Expected<Vector<BLASBuilder::Impl::BLASBuildData>, VHResult> BLASBuilder::Impl::Prepare(BLAS::Config* blasConfigs, uint32_t count, CommandBuffer& commandBuffer)
    {
        Vector<BLASBuildData> buildDataList;
        buildDataList.Resize(count);

        for (uint32_t i = 0; i < count; i++)
        {
            // Barriers for all buffers
            for (uint32_t j = 0; j < blasConfigs[i].VertexBuffers.Size(); ++j)
            {
                blasConfigs[i].VertexBuffers[j].Barrier(
                    commandBuffer,
                    VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
                    VulkanHelper::AccessFlags::SHADER_READ_BIT,
                    VulkanHelper::PipelineStages::TRANSFER_BIT,
                    VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
                );
            }

            for (uint32_t j = 0; j < blasConfigs[i].IndexBuffers.Size(); ++j)
            {
                if (blasConfigs[i].IndexBuffers[j].GetSize() > 0)
                {
                    blasConfigs[i].IndexBuffers[j].Barrier(
                        commandBuffer,
                        VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
                        VulkanHelper::AccessFlags::SHADER_READ_BIT,
                        VulkanHelper::PipelineStages::TRANSFER_BIT,
                        VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
                    );
                }
            }

            for (uint32_t j = 0; j < blasConfigs[i].VertexBuffers.Size(); ++j)
            {
                VkAccelerationStructureGeometryKHR geometry{};
                geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
                geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                
                geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                geometry.geometry.triangles.vertexData.deviceAddress = blasConfigs[i].VertexBuffers[j].GetDeviceAddress();
                geometry.geometry.triangles.vertexStride = blasConfigs[i].VertexSize;
                geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
                geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(blasConfigs[i].VertexBuffers[j].GetSize()) / blasConfigs[i].VertexSize - 1;

                if (blasConfigs[i].IndexBuffers[j] != nullptr)
                {
                    geometry.geometry.triangles.indexData.deviceAddress = blasConfigs[i].IndexBuffers[j].GetDeviceAddress();
                    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32; // Assume uint32 indices
                }
                else
                {
                    geometry.geometry.triangles.indexData.deviceAddress = 0;
                    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
                }

                buildDataList[i].geometries.PushBack(geometry);

                // Build range info
                VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
                if (blasConfigs[i].IndexBuffers[j] != nullptr)
                {
                    buildRangeInfo.primitiveCount = static_cast<uint32_t>(blasConfigs[i].IndexBuffers[j].GetSize() / sizeof(uint32_t)) / 3;
                }
                else
                {
                    buildRangeInfo.primitiveCount = static_cast<uint32_t>(blasConfigs[i].VertexBuffers[j].GetSize() / (3 * sizeof(float))) / 3;
                }
                buildRangeInfo.primitiveOffset = 0;
                buildRangeInfo.firstVertex = 0;
                buildRangeInfo.transformOffset = 0;

                buildDataList[i].buildRangeInfos.PushBack(buildRangeInfo);
                buildDataList[i].maxPrimitiveCounts.PushBack(buildRangeInfo.primitiveCount);
            }
        
            // Build info for BLAS
            VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
            buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            if (blasConfigs[i].EnableCompaction)
            {
                buildGeometryInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
            }
            buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildGeometryInfo.geometryCount = static_cast<uint32_t>(buildDataList[i].geometries.Size());
            buildGeometryInfo.pGeometries = buildDataList[i].geometries.Data();

            // Get size requirements
            VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
            buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            FunctionLoader::vkGetAccelerationStructureBuildSizesKHR(
                m_Device->GetDevice(),
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildGeometryInfo,
                buildDataList[i].maxPrimitiveCounts.Data(),
                &buildSizesInfo
            );
            buildDataList[i].buildSizesInfos.PushBack(buildSizesInfo);

            SharedPtr<Buffer::Impl> asBuffer = Buffer::Impl::New(
                m_Device,
                buildSizesInfo.accelerationStructureSize,
                Buffer::Usage::ACCELERATION_STRUCTURE_STORAGE_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
                false, // Not mappable
                m_Device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment, // Min alignment
                "BLAS Buffer"
            ).Value();
            buildDataList[i].asBuffers.PushBack(asBuffer);

            // Create the acceleration structure
            VkAccelerationStructureCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.buffer = asBuffer->GetBuffer();
            createInfo.size = buildSizesInfo.accelerationStructureSize;
            createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

            VkAccelerationStructureKHR accelerationStructure;
            VkResult result = FunctionLoader::vkCreateAccelerationStructureKHR(m_Device->GetDevice(), &createInfo, nullptr, &accelerationStructure);
            if (result != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create acceleration structure");
                return Unexpected(VHResult(result));
            }
            buildGeometryInfo.dstAccelerationStructure = accelerationStructure;

            buildDataList[i].buildGeometryInfos.PushBack(buildGeometryInfo);

            buildDataList[i].accelerationStructures.PushBack(accelerationStructure);
        }

        return buildDataList;
    }

    Expected<Vector<BLAS>, VHResult> BLASBuilder::Impl::Build(
        const Vector<BLASBuildData>& buildDataList,
        CommandBuffer& cmd
    )
    {
        Vector<BLAS> blasList;
        blasList.Reserve(buildDataList.Size());
        
        uint64_t currentBatchSize = 0;
        Vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos;
        Vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
        for (size_t i = 0; i < buildDataList.Size(); i++)
        {
            for (size_t j = 0; j < buildDataList[i].buildSizesInfos.Size(); j++)
            {
                SharedPtr<CommandBuffer::Impl> commandBuffer = CommandBuffer::Impl::GetImplementation(cmd);
                uint64_t scratchSize = buildDataList[i].buildSizesInfos[j].buildScratchSize;
                if (currentBatchSize + scratchSize > SCRATCH_BUFFER_SIZE && !buildInfos.Empty())
                {
                    FunctionLoader::vkCmdBuildAccelerationStructuresKHR(
                        commandBuffer->GetCommandBuffer(),
                        (uint32_t)buildInfos.Size(),
                        buildInfos.Data(),
                        pBuildRangeInfos.Data()
                    );

                    VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for BLAS build");
                    VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for BLAS build");
                    VH_ASSERT(cmd.BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for BLASBuilder");
                    
                    buildInfos.Clear();
                    pBuildRangeInfos.Clear();
                    currentBatchSize = 0;
                }

                buildInfos.PushBack(buildDataList[i].buildGeometryInfos[j]);
                buildInfos.Back().scratchData.deviceAddress = m_ScratchBuffer->GetDeviceAddress() + currentBatchSize;
                pBuildRangeInfos.PushBack((VkAccelerationStructureBuildRangeInfoKHR*)(&buildDataList[i].buildRangeInfos[j]));
                currentBatchSize += scratchSize;

                BLAS::Impl blas(
                    m_Device,
                    buildDataList[i].accelerationStructures[j],
                    buildDataList[i].asBuffers[j]
                );
                blasList.PushBack(BLAS::Impl::CreatePublicInterface(MakeShared<BLAS::Impl>(Move(blas))));
            }
        }

        if (!buildInfos.Empty())
        {
            SharedPtr<CommandBuffer::Impl> commandBuffer = CommandBuffer::Impl::GetImplementation(cmd);

            FunctionLoader::vkCmdBuildAccelerationStructuresKHR(
                commandBuffer->GetCommandBuffer(),
                (uint32_t)buildInfos.Size(),
                buildInfos.Data(),
                pBuildRangeInfos.Data()
            );

            VkMemoryBarrier memoryBarrier{};
            memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

            vkCmdPipelineBarrier(
                commandBuffer->GetCommandBuffer(),
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                1, &memoryBarrier,
                0, nullptr,
                0, nullptr
            );
        }

        return blasList;
    }

    VHResult BLASBuilder::Impl::Compact(Vector<BLAS>& blasList, CommandBuffer& cmd)
    {
        if (blasList.Empty())
            return VHResult::OK;

        uint32_t queryCount = (uint32_t)blasList.Size();

        VkQueryPool queryPool = VK_NULL_HANDLE;
        // Create a query pool for compaction info
        VkQueryPoolCreateInfo queryPoolCreateInfo{};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        queryPoolCreateInfo.queryCount = queryCount;

        VkResult queryResult = vkCreateQueryPool(m_Device->GetDevice(), &queryPoolCreateInfo, nullptr, &queryPool);
        if (queryResult != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create query pool for acceleration structure compaction");
            return (VHResult)queryResult;
        }
        vkResetQueryPool(m_Device->GetDevice(), queryPool, 0, queryCount);

        SharedPtr<CommandBuffer::Impl> commandBuffer = CommandBuffer::Impl::GetImplementation(cmd);

        Vector<VkAccelerationStructureKHR> accelerationStructures;
        accelerationStructures.Reserve(blasList.Size());
        for (uint32_t i = 0; i < blasList.Size(); i++)
            accelerationStructures.PushBack(BLAS::Impl::GetImplementation(blasList[i])->GetHandle());

        FunctionLoader::vkCmdWriteAccelerationStructuresPropertiesKHR(
            commandBuffer->GetCommandBuffer(),
            (uint32_t)accelerationStructures.Size(),
            accelerationStructures.Data(),
            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
            queryPool,
            0
        );

        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for compaction query");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for compaction query");
        VH_ASSERT(cmd.BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for BLASBuilder");

        // Get the compaction size
        Vector<VkDeviceSize> compactedSize;
        compactedSize.Resize(queryCount);

        vkGetQueryPoolResults(
            m_Device->GetDevice(),
            queryPool,
            0,
            queryCount,
            sizeof(VkDeviceSize) * queryCount,
            compactedSize.Data(),
            sizeof(VkDeviceSize),
            VK_QUERY_RESULT_WAIT_BIT
        );

        vkDestroyQueryPool(m_Device->GetDevice(), queryPool, nullptr);

        Vector<VkAccelerationStructureKHR> oldHandles;
        oldHandles.Resize(blasList.Size());

        for (uint32_t i = 0; i < blasList.Size(); i++)
        {
            // Create a new buffer for the compacted acceleration structure
            SharedPtr<Buffer::Impl> compactedBufferImpl = Buffer::Impl::New(
                m_Device,
                compactedSize[i],
                Buffer::Usage::ACCELERATION_STRUCTURE_STORAGE_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
                false, // Not CPU mappable
                256,   // Minimum alignment
                "Compacted BLAS Buffer"
            ).Value();

            VkAccelerationStructureCreateInfoKHR compactedCreateInfo{};
            compactedCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            compactedCreateInfo.buffer = compactedBufferImpl->GetBuffer();
            compactedCreateInfo.size = compactedSize[i];
            compactedCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

            VkAccelerationStructureKHR compactedHandle;
            VkResult createResult = FunctionLoader::vkCreateAccelerationStructureKHR(m_Device->GetDevice(), &compactedCreateInfo, nullptr, &compactedHandle);
            if (createResult != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create compacted acceleration structure");
                return VHResult(createResult);
            }

            VkCopyAccelerationStructureInfoKHR copyInfo{};
            copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
            copyInfo.src = BLAS::Impl::GetImplementation(blasList[i])->GetHandle();
            copyInfo.dst = compactedHandle;
            copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

            FunctionLoader::vkCmdCopyAccelerationStructureKHR(
                commandBuffer->GetCommandBuffer(),
                &copyInfo
            );

            // Destroy the old acceleration structure
            oldHandles[i] = BLAS::Impl::GetImplementation(blasList[i])->GetHandle();
            BLAS::Impl::GetImplementation(blasList[i])->m_Handle = compactedHandle;
            BLAS::Impl::GetImplementation(blasList[i])->m_Buffer = Move(compactedBufferImpl);
        }

        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for compaction");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for compaction");
        VH_ASSERT(cmd.BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for BLASBuilder");

        // Cleanup old acceleration structures
        for (uint32_t i = 0; i < oldHandles.Size(); i++)
        {
            FunctionLoader::vkDestroyAccelerationStructureKHR(m_Device->GetDevice(), oldHandles[i], nullptr);
        }

        return VHResult::OK;
    }

    BLASBuilder::Impl::~Impl()
    {
        // Cleanup is handled by SharedPtr destructors
    }

    BLASBuilder::Impl::Impl(Impl&& other) noexcept
        : m_Device(Move(other.m_Device))
        , m_ScratchBuffer(Move(other.m_ScratchBuffer))
    {
        other.m_Device = nullptr;
        other.m_ScratchBuffer = nullptr;
    }

    BLASBuilder::Impl& BLASBuilder::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Device = Move(other.m_Device);
        m_ScratchBuffer = Move(other.m_ScratchBuffer);

        other.m_Device = nullptr;
        other.m_ScratchBuffer = nullptr;

        return *this;
    }

    //
    //  Forward Functions
    //

    Expected<BLASBuilder, VHResult> BLASBuilder::New(const Config& config)
    {
        VH_LOG_DEBUG("Creating BLASBuilder Implementation");

        auto implResult = Impl::New(Device::Impl::GetImplementation(config.Device));
        if (!implResult.HasValue())
        {
            VH_LOG_DEBUG("Failed to create BLASBuilder implementation");
            return Unexpected(implResult.Error());
        }

        return BLASBuilder(implResult.Value());
    }

    BLASBuilder::BLASBuilder()
        : m_Impl(nullptr)
    {
    }

    BLASBuilder::BLASBuilder(BLASBuilder&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {
        other.m_Impl = nullptr;
    }

    BLASBuilder::BLASBuilder(const BLASBuilder& other)
        : m_Impl(other.m_Impl)
    {}

    BLASBuilder& BLASBuilder::operator=(const BLASBuilder& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;
        return *this;
    }

    BLASBuilder& BLASBuilder::operator=(BLASBuilder&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = Move(other.m_Impl);
        other.m_Impl = nullptr;
        return *this;
    }

    BLASBuilder::~BLASBuilder()
    {

    }

    BLASBuilder::BLASBuilder(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {}

    Expected<Vector<BLAS>, VHResult> BLASBuilder::Build(VulkanHelper::BLAS::Config* blasConfigs, uint32_t count, CommandBuffer& computeCmd)
    {
        return m_Impl->Build(blasConfigs, count, computeCmd);
    }
    
    VHResult BLASBuilder::Compact(Vector<BLAS>& blasList, CommandBuffer& commandBuffer)
    {
        return m_Impl->Compact(blasList, commandBuffer);
    }
}