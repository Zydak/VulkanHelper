#pragma once

#include "Vulkan/DescriptorPool.h"
#include "DeviceImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class DescriptorPool::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(const SharedPtr<Device::Impl>& device, uint32_t maxSets, const PoolSize* poolSizes, uint32_t poolSizeCount);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const DescriptorPool& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static DescriptorPool CreatePublicInterface(const SharedPtr<Impl>& impl) { return DescriptorPool(impl); }

        [[nodiscard]] Expected<DescriptorSet, VHResult> AllocateDescriptorSet(const DescriptorSet::Config& config);

        void Reset();

        [[nodiscard]] inline SharedPtr<Device::Impl> GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
        
    private:
        SharedPtr<Device::Impl> m_Device;
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

        explicit Impl(const SharedPtr<Device::Impl>& device, VkDescriptorPool descriptorPool)
            : m_Device(device)
            , m_DescriptorPool(descriptorPool)
        {}
    };
}