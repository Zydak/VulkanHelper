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
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            VkDescriptorSet descriptorSet,
            VkDescriptorSetLayout descriptorSetLayout,
            const DescriptorSet::Config& config
        );

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const DescriptorSet& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static DescriptorSet CreatePublicInterface(const SharedPtr<Impl>& impl) { return DescriptorSet(impl); }

        [[nodiscard]] inline SharedPtr<Device::Impl> GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }
        [[nodiscard]] inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

        [[nodiscard]] VHResult AddBuffer(uint32_t binding, uint32_t arrayIndex, const SharedPtr<Buffer::Impl>& buffer);
        [[nodiscard]] VHResult AddImage(uint32_t binding, uint32_t arrayIndex, const SharedPtr<ImageView::Impl>& imageView, Image::Layout layout);
        [[nodiscard]] VHResult AddSampler(uint32_t binding, uint32_t arrayIndex, const SharedPtr<Sampler::Impl>& sampler);
        [[nodiscard]] VHResult AddAccelerationStructure(uint32_t binding, uint32_t arrayIndex, const SharedPtr<TLAS::Impl>& accelerationStructure);

        [[nodiscard]] inline const Vector<DescriptorSet::BindingDescription>& GetBindingDescriptions() const { return m_BindingDescriptions; }
    private:
        SharedPtr<Device::Impl> m_Device;
        VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        Vector<DescriptorSet::BindingDescription> m_BindingDescriptions;

        explicit Impl(
            const SharedPtr<Device::Impl>& device,
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