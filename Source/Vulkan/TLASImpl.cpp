#include "TLASImpl.h"

#include "BufferImpl.h"

#include "FunctionLoader.h"

namespace VulkanHelper
{
    Expected<SharedPtr<TLAS::Impl>, VHResult> TLAS::Impl::New(
        const SharedPtr<Device::Impl>& device,
        const Vector<SharedPtr<BLAS::Impl>>& blasList,
        const Vector<uint32_t>& instanceCustomIndices,
        const glm::mat4* transforms,
        const SharedPtr<CommandBuffer::Impl>& commandBuffer
    )
    {
        VulkanHelper::Vector<VkAccelerationStructureInstanceKHR> instances;
        instances.Reserve(blasList.Size());

        for (uint32_t i = 0; i < blasList.Size(); i++)
        {
            SharedPtr<BLAS::Impl> const blasImpl = blasList[i];
            if (!blasImpl)
            {
                VH_LOG_ERROR("BLAS at index {} is null!", i);
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }

            VkAccelerationStructureInstanceKHR instance = {};
            instance.transform = ConvertToVulkanMatrix(transforms[i]);
            instance.instanceCustomIndex = instanceCustomIndices[i];
            instance.mask = 0xFF; // Default mask
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
            instance.accelerationStructureReference = blasImpl->GetDeviceAddress();

            instances.PushBack(instance);
        }

        auto instancesBufferRes = VulkanHelper::Buffer::Impl::New(
            device,
            instances.Size() * sizeof(VkAccelerationStructureInstanceKHR),
            VulkanHelper::Buffer::Usage::TRANSFER_DST_BIT | VulkanHelper::Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT | VulkanHelper::Buffer::Usage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT,
            false, // Not mappable
            0, // No special alignment needed
            "TLAS Instances Buffer"
        );

        if (!instancesBufferRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create instances buffer for TLAS");
            return Unexpected(instancesBufferRes.Error());
        }

        // Make a staging buffer for the instances data
        auto stagingBufferRes = VulkanHelper::Buffer::Impl::New(
            device,
            instances.Size() * sizeof(VkAccelerationStructureInstanceKHR),
            VulkanHelper::Buffer::Usage::TRANSFER_SRC_BIT,
            true, // Mappable
            1, // No special alignment needed
            "TLAS Instances Staging Buffer"
        );
        if (!stagingBufferRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create staging buffer for TLAS instances");
            return Unexpected(stagingBufferRes.Error());
        }

        SharedPtr<Buffer::Impl> instancesBuffer = Move(instancesBufferRes.Value());
        SharedPtr<Buffer::Impl> stagingBuffer = Move(stagingBufferRes.Value());

        auto res = stagingBuffer->UploadData(instances.Data(), instances.Size() * sizeof(VkAccelerationStructureInstanceKHR), 0);
        if (res != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to upload instances buffer");
            return Unexpected(res);
        }

        // Copy staging buffer to device local buffer
        res = instancesBuffer->CopyFromBuffer(commandBuffer, stagingBuffer, 0, 0, instances.Size() * sizeof(VkAccelerationStructureInstanceKHR));
        if (res != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to copy instances buffer to device local memory");
            return Unexpected(res);
        }

        instancesBuffer->Barrier(
            commandBuffer,
            VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
            VulkanHelper::AccessFlags::ACCELERATION_STRUCTURE_READ_BIT | VulkanHelper::AccessFlags::SHADER_READ_BIT,
            VulkanHelper::PipelineStages::TRANSFER_BIT,
            VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
        );
        
        VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
        instancesVk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instancesVk.data.deviceAddress = instancesBuffer->GetDeviceAddress();

        VkAccelerationStructureGeometryKHR topASGeometry{};
        topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		topASGeometry.geometry.instances = instancesVk;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &topASGeometry;
		buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        const uint32_t instanceCount = static_cast<uint32_t>(instances.Size());
		FunctionLoader::vkGetAccelerationStructureBuildSizesKHR(
            device->GetDevice(),
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo,
            &instanceCount,
            &sizeInfo
        );

        SharedPtr<Buffer::Impl> asBuffer = VulkanHelper::Buffer::Impl::New(
            device,
            sizeInfo.accelerationStructureSize,
            VulkanHelper::Buffer::Usage::ACCELERATION_STRUCTURE_STORAGE_BIT | VulkanHelper::Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not mappable
            device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment, // Min alignment
            "TLAS Buffer"
        ).Value();

        VH_ASSERT(sizeInfo.buildScratchSize <= MAX_SCRATCH_SIZE, "Scratch size for TLAS exceeds maximum allowed size");
        SharedPtr<Buffer::Impl> scratchBuffer = VulkanHelper::Buffer::Impl::New(
            device,
            sizeInfo.buildScratchSize,
            VulkanHelper::Buffer::Usage::STORAGE_BUFFER_BIT | VulkanHelper::Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not mappable
            device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment, // Min alignment
            "TLAS Scratch Buffer"
        ).Value();
        buildInfo.scratchData.deviceAddress = scratchBuffer->GetDeviceAddress();

        VkAccelerationStructureCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.buffer = asBuffer->GetBuffer();

        const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{
            .primitiveCount = instanceCount,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };

        VkAccelerationStructureKHR accelerationStructure;
        FunctionLoader::vkCreateAccelerationStructureKHR(
            device->GetDevice(),
            &createInfo,
            nullptr, // No custom allocator
            &accelerationStructure
        );

        const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &buildRangeInfo };

        buildInfo.dstAccelerationStructure = accelerationStructure;

        FunctionLoader::vkCmdBuildAccelerationStructuresKHR(
            commandBuffer->GetCommandBuffer(),
            1,
            &buildInfo,
            buildRangeInfos
        );

        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for TLAS build");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for TLAS build");
        VH_ASSERT(commandBuffer->BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for TLAS build");

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(
            device,
            accelerationStructure,
            Move(asBuffer),
            Move(blasList),
            Move(instances),
            Move(instancesBuffer),
            Move(scratchBuffer)
        )));
    }

    VHResult TLAS::Impl::Update(const glm::mat4* transforms, uint32_t transformCount, VulkanHelper::CommandBuffer& commandBuffer)
    {
        for (uint32_t i = 0; i < m_Instances.Size(); i++)
        {
            m_Instances[i].transform = ConvertToVulkanMatrix(transforms[i]);
        }

        // Make a staging buffer for the instances data
        auto stagingBufferRes = VulkanHelper::Buffer::Impl::New(
            m_Device,
            m_Instances.Size() * sizeof(VkAccelerationStructureInstanceKHR),
            VulkanHelper::Buffer::Usage::TRANSFER_SRC_BIT,
            true, // Mappable
            1, // No special alignment needed
            "TLAS Instances Staging Buffer"
        );
        if (!stagingBufferRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create staging buffer for TLAS instances");
            return stagingBufferRes.Error();
        }

        SharedPtr<Buffer::Impl> stagingBuffer = Move(stagingBufferRes.Value());

        auto res = stagingBuffer->UploadData(m_Instances.Data(), m_Instances.Size() * sizeof(VkAccelerationStructureInstanceKHR), 0);
        if (res != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to upload instances buffer");
            return res;
        }

        // Copy staging buffer to device local buffer
        res = m_InstancesBuffer->CopyFromBuffer(
            CommandBuffer::Impl::GetImplementation(commandBuffer),
            stagingBuffer,
            0,
            0,
            m_Instances.Size() * sizeof(VkAccelerationStructureInstanceKHR)
        );

        if (res != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to copy instances buffer to device local memory");
            return res;
        }

        m_InstancesBuffer->Barrier(
            CommandBuffer::Impl::GetImplementation(commandBuffer),
            VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
            VulkanHelper::AccessFlags::ACCELERATION_STRUCTURE_READ_BIT | VulkanHelper::AccessFlags::SHADER_READ_BIT,
            VulkanHelper::PipelineStages::TRANSFER_BIT,
            VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
        );

        VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
        instancesVk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instancesVk.data.deviceAddress = m_InstancesBuffer->GetDeviceAddress();

        uint64_t instanceCount = m_Instances.Size();

        if (transformCount != instanceCount)
        {
            VH_LOG_ERROR("Transform count does not match instance count in TLAS update");
            return VHResult::WRONG_ARGUMENTS;
        }

        VkAccelerationStructureGeometryKHR topASGeometry{};
        topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		topASGeometry.geometry.instances = instancesVk;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		buildInfo.geometryCount = 1;
		buildInfo.pGeometries = &topASGeometry;
		buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;
		buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        
		buildInfo.srcAccelerationStructure = m_Handle;
        buildInfo.dstAccelerationStructure = m_Handle;

        buildInfo.scratchData.deviceAddress = m_ScratchBuffer->GetDeviceAddress();

        const VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{
            .primitiveCount = (uint32_t)instanceCount,
            .primitiveOffset = 0,
            .firstVertex = 0,
            .transformOffset = 0
        };

        const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos[] = { &buildRangeInfo };

        FunctionLoader::vkCmdBuildAccelerationStructuresKHR(
            CommandBuffer::Impl::GetImplementation(commandBuffer)->GetCommandBuffer(),
            1,
            &buildInfo,
            buildRangeInfos
        );

        return VHResult::OK;
    }

    VkTransformMatrixKHR TLAS::Impl::ConvertToVulkanMatrix(const glm::mat4& mat)
    {
        VkTransformMatrixKHR vkMat = {};
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                vkMat.matrix[i][j] = mat[j][i];
            }
        }
        return vkMat;
    }

    TLAS::Impl::~Impl()
    {
        if (m_Handle != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying TLAS Implementation");
            FunctionLoader::vkDestroyAccelerationStructureKHR(m_Device->GetDevice(), m_Handle, nullptr);
        }
    }

    TLAS::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Handle(other.m_Handle)
        , m_Buffer(VulkanHelper::Move(other.m_Buffer))
        , m_BlasList(Move(other.m_BlasList))
        , m_Instances(Move(other.m_Instances))
        , m_InstancesBuffer(VulkanHelper::Move(other.m_InstancesBuffer))
        , m_ScratchBuffer(VulkanHelper::Move(other.m_ScratchBuffer))
    {
        other.m_Handle = VK_NULL_HANDLE;
        other.m_Instances.Clear();
        other.m_InstancesBuffer = nullptr;
        other.m_BlasList.Clear();
        other.m_Device = nullptr;
        other.m_Buffer = nullptr;
        other.m_ScratchBuffer = nullptr;
    }

    TLAS::Impl& TLAS::Impl::operator=(Impl&& other) noexcept
    {
        if (this != &other)
        {
            m_Device = other.m_Device;
            m_Handle = other.m_Handle;
            m_Buffer = VulkanHelper::Move(other.m_Buffer);
            m_BlasList = Move(other.m_BlasList);
            m_Instances = Move(other.m_Instances);
            m_InstancesBuffer = VulkanHelper::Move(other.m_InstancesBuffer);
            m_ScratchBuffer = VulkanHelper::Move(other.m_ScratchBuffer);

            other.m_Handle = VK_NULL_HANDLE;
            other.m_Instances.Clear();
            other.m_InstancesBuffer = nullptr;
            other.m_BlasList.Clear();
            other.m_Device = nullptr;
            other.m_Buffer = nullptr;
            other.m_ScratchBuffer = nullptr;
        }
        return *this;
    }

    //
    //  Forward Functions
    //

    Expected<TLAS, VHResult> TLAS::New(const Config& config)
    {
        VH_LOG_INFO("Creating TLAS Implementation");

        if (config.BlasList.Empty())
        {
            VH_LOG_ERROR("BLAS list is null or empty in TLAS configuration");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.CommandBuffer == nullptr)
        {
            VH_LOG_ERROR("CommandBuffer is null in TLAS configuration");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<SharedPtr<BLAS::Impl>> blasList;
        blasList.Reserve(config.BlasList.Size());
        for (uint32_t i = 0; i < config.BlasList.Size(); i++)
        {
            blasList.PushBack(BLAS::Impl::GetImplementation(config.BlasList[i]));
        }

        auto implRes = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            blasList,
            config.InstanceCustomIndices,
            config.Transforms,
            CommandBuffer::Impl::GetImplementation(*config.CommandBuffer)
        );

        if (!implRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create TLAS implementation");
            return Unexpected(implRes.Error());
        }

        return TLAS{ VulkanHelper::Move(implRes.Value()) };
    }

    TLAS::TLAS()
        : m_Impl(nullptr)
    {
    }

    TLAS::TLAS(const TLAS& other)
        : m_Impl(other.m_Impl)
    {
    }

    TLAS& TLAS::operator=(const TLAS& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;
        return *this;
    }

    TLAS::TLAS(TLAS&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {
        other.m_Impl = nullptr;
    }

    TLAS& TLAS::operator=(TLAS&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = Move(other.m_Impl);
        return *this;
    }

    TLAS::~TLAS()
    {
        m_Impl = nullptr;
    }

    TLAS::TLAS(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {

    }

    VHResult TLAS::Update(const glm::mat4* transforms, uint32_t count, VulkanHelper::CommandBuffer& commandBuffer)
    {
        return m_Impl->Update(transforms, count, commandBuffer);
    }
}