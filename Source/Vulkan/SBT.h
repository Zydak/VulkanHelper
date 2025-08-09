#pragma once

#include "DeviceImpl.h"
#include "BufferImpl.h"
#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    class SBT
    {
    public:
        [[nodiscard]] static Expected<SBT, VHResult> New(Device::Impl* device, VkPipeline rtPipeline, uint32_t rgenCount, uint32_t missCount, uint32_t hitGroupCount, CommandBuffer& cmd);

        SBT(const SBT& other) = delete;
        SBT& operator=(const SBT& other) = delete;

        SBT(SBT&& other) noexcept;
        SBT& operator=(SBT&& other) noexcept;

        ~SBT();

        [[nodiscard]] inline Buffer::Impl* GetSBTBuffer() { return &m_SBTBuffer; }
        [[nodiscard]] inline VkStridedDeviceAddressRegionKHR GetRgenRegion() const { return m_RgenRegion; }
        [[nodiscard]] inline VkStridedDeviceAddressRegionKHR GetMissRegion() const { return m_MissRegion; }
        [[nodiscard]] inline VkStridedDeviceAddressRegionKHR GetHitRegion() const { return m_HitRegion; }

    private:
        Buffer::Impl m_SBTBuffer;

		VkStridedDeviceAddressRegionKHR m_RgenRegion{};
		VkStridedDeviceAddressRegionKHR m_MissRegion{};
		VkStridedDeviceAddressRegionKHR m_HitRegion{};

        SBT(
            Buffer::Impl&& sbtBuffer,
            VkStridedDeviceAddressRegionKHR rgenRegion,
            VkStridedDeviceAddressRegionKHR missRegion,
            VkStridedDeviceAddressRegionKHR hitRegion
        )
            : m_SBTBuffer(VulkanHelper::Move(sbtBuffer)),
              m_RgenRegion(rgenRegion),
              m_MissRegion(missRegion),
              m_HitRegion(hitRegion)
        {}
    };
}