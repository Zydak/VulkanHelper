#include "TLASImpl.h"

#include "BufferImpl.h"

#include "FunctionLoader.h"

namespace VulkanHelper
{
    Expected<SharedPtr<TLAS::Impl>, VHResult> TLAS::Impl::New(
        const SharedPtr<Device::Impl>& device,
        const Vector<SharedPtr<BLAS::Impl>>& blasList,
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
            instance.instanceCustomIndex = i;
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
            false,
            false,
            device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment,
            "TLAS Instances Buffer"
        );

        if (!instancesBufferRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create instances buffer for TLAS");
            return Unexpected(instancesBufferRes.Error());
        }

        SharedPtr<Buffer::Impl> instancesBuffer = Move(instancesBufferRes.Value());
        instancesBuffer->UploadData(instances.Data(), instances.Size() * sizeof(VkAccelerationStructureInstanceKHR), 0, commandBuffer);
        instancesBuffer->Barrier(
            commandBuffer,
            VulkanHelper::AccessFlags::TRANSFER_WRITE_BIT,
            VulkanHelper::AccessFlags::SHADER_READ_BIT,
            VulkanHelper::PipelineStages::TRANSFER_BIT,
            VulkanHelper::PipelineStages::ACCELERATION_STRUCTURE_BUILD_BIT_KHR
        );

        VH_ASSERT(commandBuffer->EndRecording() == VHResult::OK, "Failed to end command buffer recording for TLAS creation");
        VH_ASSERT(commandBuffer->SubmitAndWait() == VHResult::OK, "Failed to submit command buffer for TLAS creation");
        VH_ASSERT(commandBuffer->BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VHResult::OK, "Failed to begin command buffer recording for TLAS creation");

        VkAccelerationStructureGeometryInstancesDataKHR instancesVk{};
        instancesVk.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		instancesVk.data.deviceAddress = instancesBuffer->GetDeviceAddress();

        VkAccelerationStructureGeometryKHR topASGeometry{};
        topASGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		topASGeometry.geometry.instances = instancesVk;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
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
            false, // No persistent staging
            device->GetAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment, // Min alignment
            "TLAS Buffer"
        ).Value();

        VH_ASSERT(sizeInfo.buildScratchSize <= MAX_SCRATCH_SIZE, "Scratch size for TLAS exceeds maximum allowed size");
        SharedPtr<Buffer::Impl> scratchBuffer = VulkanHelper::Buffer::Impl::New(
            device,
            sizeInfo.buildScratchSize,
            VulkanHelper::Buffer::Usage::STORAGE_BUFFER_BIT | VulkanHelper::Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not mappable
            false, // No persistent staging
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

        return SharedPtr( new Impl(
            device,
            accelerationStructure,
            VulkanHelper::Move(asBuffer)
        ));
    }

    VkTransformMatrixKHR TLAS::Impl::ConvertToVulkanMatrix(const glm::mat4& mat)
    {
        VkTransformMatrixKHR vkMat = {};
        glm::mat4 transposeMat = glm::transpose(mat); // VkTransformMatrixKHR is row-major, but glm is column-major
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                vkMat.matrix[i][j] = transposeMat[i][j];
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
        : m_Device(other.m_Device),
          m_Handle(other.m_Handle),
          m_Buffer(VulkanHelper::Move(other.m_Buffer))
    {
        other.m_Handle = VK_NULL_HANDLE;
    }

    TLAS::Impl& TLAS::Impl::operator=(Impl&& other) noexcept
    {
        if (this != &other)
        {
            m_Device = other.m_Device;
            m_Handle = other.m_Handle;
            m_Buffer = VulkanHelper::Move(other.m_Buffer);
            other.m_Handle = VK_NULL_HANDLE;
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

        this->~TLAS(); // Clean up current state

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

        this->~TLAS(); // Clean up current state

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
}