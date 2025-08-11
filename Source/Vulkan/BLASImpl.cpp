#include "BLASImpl.h"

#include <vulkan/vulkan.h>
#include <cstring>
#include <algorithm>

#include "Core/Move.h"
#include "DeviceImpl.h"
#include "BufferImpl.h"
#include "Vulkan/CommandBuffer.h"
#include "CommandBufferImpl.h"
#include "Log/Log.h"

#include "FunctionLoader.h"

namespace VulkanHelper
{
    Expected<BLAS, VHResult> BLAS::Impl::New(
        Device::Impl* device,
        const Vector<Buffer::Impl*>& vertexBuffers,
        const Vector<Buffer::Impl*>& indexBuffers,
        bool enableCompaction,
        CommandBuffer::Impl* commandBuffer
    )
    {
        VH_LOG_INFO("Creating Vulkan Acceleration Structure Implementation");

        for (uint32_t i = 0; i < vertexBuffers.Size(); ++i)
        {
            vertexBuffers[i]->Barrier(
                commandBuffer,
                VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
                VulkanHelper::AccessFlags::SHADER_READ_BIT,
                VulkanHelper::PipelineStages::TRANSFER_BIT,
                VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
            );
        }

        for (uint32_t i = 0; i < indexBuffers.Size(); ++i)
        {
            if (indexBuffers[i]->GetSize() > 0)
            {
                indexBuffers[i]->Barrier(
                    commandBuffer,
                    VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
                    VulkanHelper::AccessFlags::SHADER_READ_BIT,
                    VulkanHelper::PipelineStages::TRANSFER_BIT,
                    VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
                );
            }
        }

        // Build geometry descriptions
        VulkanHelper::Vector<VkAccelerationStructureGeometryKHR> geometries;
        VulkanHelper::Vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
        VulkanHelper::Vector<uint32_t> maxPrimitiveCounts;

        geometries.Reserve(vertexBuffers.Size());
        buildRangeInfos.Reserve(vertexBuffers.Size());
        maxPrimitiveCounts.Reserve(vertexBuffers.Size());

        for (uint32_t i = 0; i < vertexBuffers.Size(); ++i)
        {
            VkAccelerationStructureGeometryKHR geometry{};
            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            
            geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexData.deviceAddress = vertexBuffers[i]->GetDeviceAddress();
            geometry.geometry.triangles.vertexStride = 3 * sizeof(float); // Assume vec3 positions for now
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(vertexBuffers[i]->GetSize() / (3 * sizeof(float))) - 1;

            if (indexBuffers[i] != nullptr)
            {
                geometry.geometry.triangles.indexData.deviceAddress = indexBuffers[i]->GetDeviceAddress();
                geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32; // Assume uint32 indices
            }
            else
            {
                geometry.geometry.triangles.indexData.deviceAddress = 0;
                geometry.geometry.triangles.indexType = VK_INDEX_TYPE_NONE_KHR;
            }

            geometries.PushBack(geometry);

            // Build range info
            VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
            if (indexBuffers[i] != nullptr)
            {
                buildRangeInfo.primitiveCount = static_cast<uint32_t>(indexBuffers[i]->GetSize() / sizeof(uint32_t)) / 3;
            }
            else
            {
                buildRangeInfo.primitiveCount = static_cast<uint32_t>(vertexBuffers[i]->GetSize() / (3 * sizeof(float))) / 3;
            }
            buildRangeInfo.primitiveOffset = 0;
            buildRangeInfo.firstVertex = 0;
            buildRangeInfo.transformOffset = 0;

            buildRangeInfos.PushBack(buildRangeInfo);
            maxPrimitiveCounts.PushBack(buildRangeInfo.primitiveCount);
        }

        // Build info for BLAS
        VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
        buildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        if (enableCompaction)
        {
            buildGeometryInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        }
        buildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.Size());
        buildGeometryInfo.pGeometries = geometries.Data();

        // Get size requirements
        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

        FunctionLoader::vkGetAccelerationStructureBuildSizesKHR(
            device->GetDevice(),
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildGeometryInfo,
            maxPrimitiveCounts.Data(),
            &buildSizesInfo
        );
        
        Buffer::Impl asBuffer = Buffer::Impl::New(
            device,
            buildSizesInfo.accelerationStructureSize,
            Buffer::Usage::ACCELERATION_STRUCTURE_STORAGE_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not mappable
            false, // No persistent staging
            device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment, // Min alignment
            "BLAS Buffer"
        ).Value();

        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        createInfo.buffer = asBuffer.GetBuffer();
        createInfo.size = buildSizesInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

        VkAccelerationStructureKHR accelerationStructure;
        VkResult result = FunctionLoader::vkCreateAccelerationStructureKHR(device->GetDevice(), &createInfo, nullptr, &accelerationStructure);
        if (result != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create acceleration structure");
            return Unexpected(VHResult(result));
        }

        VH_ASSERT(buildSizesInfo.buildScratchSize <= BLAS_MAX_SIZE, "BLAS scratch size exceeds maximum allowed size");

        // Update build geometry info with scratch buffer address
        buildGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        buildGeometryInfo.dstAccelerationStructure = accelerationStructure;

        VkAccelerationStructureKHR finalAccelerationStructure = accelerationStructure;

        auto impl = new Impl(
            device,
            finalAccelerationStructure,
            Move(asBuffer),
            buildSizesInfo.buildScratchSize
        );

        VH_ASSERT(impl->Build(commandBuffer, buildRangeInfos.Data(), buildGeometryInfo) == VHResult::OK, "Failed to build acceleration structure");

        if (enableCompaction)
        {
            VH_ASSERT(impl->Compact(commandBuffer) == VHResult::OK, "Failed to compact acceleration structure");
        }
        
        return Impl::CreatePublicInterface(UniquePtr<Impl>(impl));
    }

    BLAS::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Handle(other.m_Handle)
        , m_Buffer(Move(other.m_Buffer))
        , m_ScratchBufferSize(other.m_ScratchBufferSize)
    {
        other.m_Device = nullptr;
        other.m_Handle = VK_NULL_HANDLE;
    }

    BLAS::Impl& BLAS::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        // Clean up current resources
        this->~Impl();

        // Move from other
        m_Device = other.m_Device;
        m_Handle = other.m_Handle;
        m_Buffer = Move(other.m_Buffer);
        m_ScratchBufferSize = other.m_ScratchBufferSize;

        // Reset other
        other.m_Device = nullptr;
        other.m_Handle = VK_NULL_HANDLE;
        other.m_ScratchBufferSize = 0;

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
        m_ScratchBufferSize = 0;
    }

    VkDeviceAddress BLAS::Impl::GetDeviceAddress() const
    {
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
        addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addressInfo.accelerationStructure = m_Handle;

        return FunctionLoader::vkGetAccelerationStructureDeviceAddressKHR(m_Device->GetDevice(), &addressInfo);
    }

    VHResult BLAS::Impl::Build(
        CommandBuffer::Impl* commandBuffer,
        VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos,
        VkAccelerationStructureBuildGeometryInfoKHR& buildInfo
    )
    {
        if (!commandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null for acceleration structure building");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (!m_Device || m_Handle == VK_NULL_HANDLE)
        {
            VH_LOG_ERROR("Invalid acceleration structure for building");
            return VHResult::WRONG_ARGUMENTS;
        }

        Buffer::Impl scratchBuffer = (Buffer::Impl::New(
            m_Device,
            m_ScratchBufferSize,
            Buffer::Usage::STORAGE_BUFFER_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not mappable
            false, // No persistent staging
            m_Device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment,
            "BLAS Scratch Buffer"
        ).Value());

        // Update build geometry info to point to current geometry data
        buildInfo.dstAccelerationStructure = m_Handle;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        // Prepare build range info pointers
        const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfos = buildRangeInfos;

        // Record build command
        FunctionLoader::vkCmdBuildAccelerationStructuresKHR(
            commandBuffer->GetCommandBuffer(),
            1,
            &buildInfo,
            &pBuildRangeInfos
        );

        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for compaction");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for compaction");
        VH_ASSERT(commandBuffer->BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for compaction");

        VH_LOG_INFO("Recorded acceleration structure build command into command buffer");
        
        return VHResult::OK;
    }

    VHResult BLAS::Impl::Compact(CommandBuffer::Impl* commandBuffer)
    {
        if (!commandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null for acceleration structure compaction");
            return VHResult::WRONG_ARGUMENTS;
        }

        if (!m_Device || m_Handle == VK_NULL_HANDLE)
        {
            VH_LOG_ERROR("Invalid acceleration structure for compaction");
            return VHResult::WRONG_ARGUMENTS;
        }

        // Create a query pool for compaction info
        VkQueryPoolCreateInfo queryPoolCreateInfo{};
        queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolCreateInfo.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        queryPoolCreateInfo.queryCount = 1;

        VkQueryPool queryPool;
        VkResult queryResult = vkCreateQueryPool(m_Device->GetDevice(), &queryPoolCreateInfo, nullptr, &queryPool);
        if (queryResult != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create query pool for acceleration structure compaction");
            return VHResult(queryResult);
        }
        vkResetQueryPool(m_Device->GetDevice(), queryPool, 0, 1);

        FunctionLoader::vkCmdWriteAccelerationStructuresPropertiesKHR(
            commandBuffer->GetCommandBuffer(),
            1,
            &m_Handle,
            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
            queryPool,
            0
        );
        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for compaction");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for compaction");
        VH_ASSERT(commandBuffer->BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for compaction");

        // Get the compaction size
        VkDeviceSize compactedSize = 0;

        vkGetQueryPoolResults(
            m_Device->GetDevice(),
            queryPool,
            0,
            1,
            sizeof(VkDeviceSize),
            &compactedSize,
            sizeof(VkDeviceSize),
            VK_QUERY_RESULT_WAIT_BIT
        );

        vkDestroyQueryPool(m_Device->GetDevice(), queryPool, nullptr);

        if (compactedSize == 0)
        {
            VH_LOG_ERROR("Compaction query returned size of 0, compaction failed");
            return VHResult::UNKNOWN;
        }

        // Create a new buffer for the compacted acceleration structure
        Buffer::Impl compactedBufferImpl = Buffer::Impl::New(
            m_Device,
            compactedSize,
            Buffer::Usage::ACCELERATION_STRUCTURE_STORAGE_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not CPU mappable
            false, // No persistent staging buffer
            256,   // Minimum alignment
            "Compacted BLAS Buffer"
        ).Value();

        VkAccelerationStructureCreateInfoKHR compactedCreateInfo{};
        compactedCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        compactedCreateInfo.buffer = compactedBufferImpl.GetBuffer();
        compactedCreateInfo.size = compactedSize;
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
        copyInfo.src = m_Handle;
        copyInfo.dst = compactedHandle;
        copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;

        FunctionLoader::vkCmdCopyAccelerationStructureKHR(
            commandBuffer->GetCommandBuffer(),
            &copyInfo
        );

        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for compaction");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for compaction");
        VH_ASSERT(commandBuffer->BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for compaction");

        // Destroy the old acceleration structure
        FunctionLoader::vkDestroyAccelerationStructureKHR(m_Device->GetDevice(), m_Handle, nullptr);
        m_Handle = compactedHandle;

        return VHResult::OK;
    }

    //
    //  Forward Functions
    //

    Expected<BLAS, VHResult> BLAS::New(const Config& config)
    {
        VH_LOG_INFO("Creating BLAS Implementation");

        if (!config.Device)
        {
            VH_LOG_ERROR("Device cannot be null!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.VertexBuffers.Empty())
        {
            VH_LOG_ERROR("VertexBuffers cannot be empty!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.IndexBuffers.Size() != config.VertexBuffers.Size())
        {
            VH_LOG_ERROR("IndexBufferCount must match VertexBufferCount when IndexBuffers are provided!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (!config.CommandBuffer)
        {
            VH_LOG_ERROR("CommandBuffer cannot be null!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<Buffer::Impl*> vertexBuffers;
        vertexBuffers.Reserve(config.VertexBuffers.Size());
        for (auto* buffer : config.VertexBuffers)
            vertexBuffers.PushBack(Buffer::Impl::GetImplementation(buffer));

        VulkanHelper::Vector<Buffer::Impl*> indexBuffers;
        indexBuffers.Reserve(config.IndexBuffers.Size());
        for (auto* buffer : config.IndexBuffers)
            indexBuffers.PushBack(Buffer::Impl::GetImplementation(buffer));

        return Impl::New(
            Device::Impl::GetImplementation(config.Device),
            vertexBuffers,
            indexBuffers,
            config.EnableCompaction,
            CommandBuffer::Impl::GetImplementation(config.CommandBuffer)
        );
    }

    BLAS::BLAS(BLAS&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {}

    BLAS& BLAS::operator=(BLAS&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~BLAS(); // Clean up current state
        m_Impl = Move(other.m_Impl);
        return *this;
    }

    BLAS::~BLAS()
    {

    }

    BLAS::BLAS(UniquePtr<Impl>&& impl)
        : m_Impl(Move(impl))
    {}
}
