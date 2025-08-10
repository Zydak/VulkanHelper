#include "SBT.h"

#include "Utility/Utility.h"
#include <vector>

#include "FunctionLoader.h"

namespace VulkanHelper
{
    Expected<SBT, VHResult> SBT::New(Device::Impl* device, VkPipeline rtPipeline, uint32_t rgenCount, uint32_t missCount, uint32_t hitGroupCount, CommandBuffer& cmd)
    {
        VH_LOG_INFO("Creating SBT Implementation");

        if (!device)
        {
            VH_LOG_ERROR("Device cannot be null!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        const uint32_t handleCount = rgenCount + missCount + hitGroupCount;
        const uint32_t handleSize = device->GetRayTracingProperties().shaderGroupHandleSize;
        const uint32_t handleAlignment = device->GetRayTracingProperties().shaderGroupHandleAlignment;
        const uint32_t bufferBaseAlignment = device->GetRayTracingProperties().shaderGroupBaseAlignment;

        const uint32_t handleSizeAligned = VulkanHelper::GetAlignment(handleSize, handleAlignment);
        
		VkStridedDeviceAddressRegionKHR rgenRegion;
        rgenRegion.size = VulkanHelper::GetAlignment(handleSizeAligned * rgenCount, bufferBaseAlignment);
        rgenRegion.stride = VulkanHelper::GetAlignment(handleSizeAligned, bufferBaseAlignment);

        VkStridedDeviceAddressRegionKHR missRegion;
        missRegion.size = VulkanHelper::GetAlignment(handleSizeAligned * missCount, bufferBaseAlignment);
        missRegion.stride = VulkanHelper::GetAlignment(handleSizeAligned, bufferBaseAlignment);

        VkStridedDeviceAddressRegionKHR hitRegion;
        hitRegion.size = VulkanHelper::GetAlignment(handleSizeAligned * hitGroupCount, bufferBaseAlignment);
        hitRegion.stride = VulkanHelper::GetAlignment(handleSizeAligned, bufferBaseAlignment);

        std::vector<uint8_t> handleData(handleCount * handleSize, 0);
        VkResult result = FunctionLoader::vkGetRayTracingShaderGroupHandlesKHR(
            device->GetDevice(),
            rtPipeline,
            0,
            handleCount,
            static_cast<size_t>(handleData.size()),
            handleData.data()
        );
        if (result != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get ray tracing shader group handles");
            return Unexpected(VHResult(result));
        }

        const uint32_t sbtSize = rgenRegion.size + missRegion.size + hitRegion.size;

        VulkanHelper::Buffer::Impl stagingBuffer = VulkanHelper::Buffer::Impl::New(
            device,
            sbtSize,
            VulkanHelper::Buffer::Usage::TRANSFER_SRC_BIT | VulkanHelper::Buffer::Usage::SHADER_BINDING_TABLE_BIT,
            true, // CPU mappable for staging
            false, // No persistent staging buffer
            bufferBaseAlignment, // Minimum alignment
            "SBT Staging Buffer"
        ).Value();

        auto mappedDataRes = stagingBuffer.Map();
        if (!mappedDataRes.HasValue())
        {
            VH_LOG_ERROR("Failed to map staging buffer for SBT");
            return Unexpected(mappedDataRes.Error());
        }

        uint8_t* mappedData = (uint8_t*)mappedDataRes.Value();

        uint32_t handleIndex = 0;
        for (uint32_t i = 0; i < rgenCount; ++i)
        {
            std::memcpy(mappedData + i * rgenRegion.stride, handleData.data() + handleIndex * handleSize, handleSize);
            handleIndex++;
        }
        mappedData += rgenRegion.size;

        for (uint32_t i = 0; i < missCount; ++i)
        {
            std::memcpy(mappedData + i * missRegion.stride, handleData.data() + handleIndex * handleSize, handleSize);
            handleIndex++;
        }
        mappedData += missRegion.size;

        for (uint32_t i = 0; i < hitGroupCount; ++i)
        {
            std::memcpy(mappedData + i * hitRegion.stride, handleData.data() + handleIndex * handleSize, handleSize);
            handleIndex++;
        }
        mappedData += hitRegion.size;

        stagingBuffer.Unmap();

        VulkanHelper::Buffer::Impl sbtBuffer = VulkanHelper::Buffer::Impl::New(
            device,
            sbtSize,
            Buffer::Usage::SHADER_BINDING_TABLE_BIT | Buffer::Usage::TRANSFER_DST_BIT | Buffer::Usage::SHADER_DEVICE_ADDRESS_BIT,
            false, // Not CPU mappable
            false, // No persistent staging buffer
            bufferBaseAlignment, // Minimum alignment
            "SBT Buffer"
        ).Value();

        sbtBuffer.CopyFrom(cmd, stagingBuffer, 0, 0, sbtSize);

        rgenRegion.deviceAddress = sbtBuffer.GetDeviceAddress();
        missRegion.deviceAddress = rgenRegion.deviceAddress + rgenRegion.size;
        hitRegion.deviceAddress = missRegion.deviceAddress + missRegion.size;

        return SBT(
            VulkanHelper::Move(sbtBuffer),
            rgenRegion,
            missRegion,
            hitRegion
        );
    }

    SBT::SBT(SBT&& other) noexcept
        : m_SBTBuffer(VulkanHelper::Move(other.m_SBTBuffer)),
          m_RgenRegion(other.m_RgenRegion),
          m_MissRegion(other.m_MissRegion),
          m_HitRegion(other.m_HitRegion)
    {}

    SBT& SBT::operator=(SBT&& other) noexcept
    {
        if (this == &other)
            return *this;

        // Clean up current resources
        this->~SBT();

        // Move from other
        m_SBTBuffer = VulkanHelper::Move(other.m_SBTBuffer);
        m_RgenRegion = other.m_RgenRegion;
        m_MissRegion = other.m_MissRegion;
        m_HitRegion = other.m_HitRegion;

        return *this;
    }

    SBT::~SBT()
    {
        
    }
}