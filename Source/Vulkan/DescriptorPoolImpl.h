#pragma once

#include "Vulkan/DescriptorPool.h"
#include "DeviceImpl.h"

typedef struct VkDescriptorPool_T* VkDescriptorPool;

namespace VulkanHelper
{
    class DescriptorPool::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(Device::Impl* device, uint32_t maxSets, const PoolSize* poolSizes, uint32_t poolSizeCount);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const DescriptorPool* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static DescriptorPool CreatePublicInterface(VulkanHelper::UniquePtr<Impl>&& impl) { return DescriptorPool(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }

        [[nodiscard]] VulkanHelper::Expected<DescriptorSet, VHResult> AllocateDescriptorSet(const DescriptorSet::Config& config);

        void Reset();
    private:
        Device::Impl* m_Device;
        VkDescriptorPool m_DescriptorPool = nullptr;

        explicit Impl(Device::Impl* device, VkDescriptorPool descriptorPool)
            : m_Device(device)
            , m_DescriptorPool(descriptorPool)
        {}
    };
}