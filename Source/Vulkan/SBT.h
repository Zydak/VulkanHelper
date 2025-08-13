#pragma once

#include "DeviceImpl.h"
#include "BufferImpl.h"
#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    class SBT
    {
    public:
        [[nodiscard]] static Expected<SBT, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            VkPipeline rtPipeline,
            uint32_t rgenCount,
            uint32_t missCount,
            uint32_t hitGroupCount,
            const SharedPtr<CommandBuffer::Impl> cmd
        );

        SBT(const SBT& other) = delete;
        SBT& operator=(const SBT& other) = delete;

        SBT(SBT&& other) noexcept;
        SBT& operator=(SBT&& other) noexcept;

        ~SBT();

        [[nodiscard]] inline SharedPtr<Buffer::Impl> GetSBTBuffer() { return m_SBTBuffer; }
        [[nodiscard]] inline VkStridedDeviceAddressRegionKHR GetRgenRegion() const { return m_RgenRegion; }
        [[nodiscard]] inline VkStridedDeviceAddressRegionKHR GetMissRegion() const { return m_MissRegion; }
        [[nodiscard]] inline VkStridedDeviceAddressRegionKHR GetHitRegion() const { return m_HitRegion; }

        [[nodiscard]] inline const VkStridedDeviceAddressRegionKHR* GetRgenRegionPtr() const { return &m_RgenRegion; }
        [[nodiscard]] inline const VkStridedDeviceAddressRegionKHR* GetMissRegionPtr() const { return &m_MissRegion; }
        [[nodiscard]] inline const VkStridedDeviceAddressRegionKHR* GetHitRegionPtr() const { return &m_HitRegion; }

    private:
        SharedPtr<Buffer::Impl> m_SBTBuffer;

		VkStridedDeviceAddressRegionKHR m_RgenRegion{};
		VkStridedDeviceAddressRegionKHR m_MissRegion{};
		VkStridedDeviceAddressRegionKHR m_HitRegion{};

        SBT(
            const SharedPtr<Buffer::Impl>& sbtBuffer,
            const VkStridedDeviceAddressRegionKHR& rgenRegion,
            const VkStridedDeviceAddressRegionKHR& missRegion,
            const VkStridedDeviceAddressRegionKHR& hitRegion
        )
            : m_SBTBuffer(sbtBuffer),
              m_RgenRegion(rgenRegion),
              m_MissRegion(missRegion),
              m_HitRegion(hitRegion)
        {}
    };
}