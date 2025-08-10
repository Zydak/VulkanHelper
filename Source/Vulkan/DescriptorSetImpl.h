#pragma once

#include "Vulkan/DescriptorSet.h"
#include "DeviceImpl.h"
#include "BufferImpl.h"
#include "ImageViewImpl.h"
#include "SamplerImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class DescriptorSet::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(Device::Impl* device, VkDescriptorSet descriptorSet, VkDescriptorSetLayout descriptorSetLayout, const DescriptorSet::Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const DescriptorSet* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static DescriptorSet CreatePublicInterface(VulkanHelper::UniquePtr<Impl>&& impl) { return DescriptorSet(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
        [[nodiscard]] inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

        [[nodiscard]] VHResult AddBuffer(uint32_t binding, uint32_t arrayIndex, const Buffer& buffer);
        [[nodiscard]] VHResult AddImage(uint32_t binding, uint32_t arrayIndex, const ImageView& imageView, Image::Layout layout);
        [[nodiscard]] VHResult AddSampler(uint32_t binding, uint32_t arrayIndex, const Sampler& sampler);

    private:
        Device::Impl* m_Device;
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        Vector<DescriptorSet::BindingDescription> m_BindingDescriptions;

        explicit Impl(
            Device::Impl* device,
            VkDescriptorSet descriptorSet,
            VkDescriptorSetLayout descriptorSetLayout,
            Vector<DescriptorSet::BindingDescription>&& bindingDescriptions
        )
            : m_Device(device)
            , m_DescriptorSet(descriptorSet)
            , m_DescriptorSetLayout(descriptorSetLayout)
            , m_BindingDescriptions(Move(bindingDescriptions))
        {}
    };
}