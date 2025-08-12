#include "Vulkan/DescriptorSet.h"
#include "DescriptorSetImpl.h"

#include "Log/Log.h"
#include "Vulkan/Device.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/ImageView.h"
#include "Vulkan/Sampler.h"
#include "Core/Error.h"

#include "Vulkan/TLASImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<DescriptorSet::Impl>, VHResult> DescriptorSet::Impl::New(Device::Impl* device, VkDescriptorSet descriptorSet, VkDescriptorSetLayout descriptorSetLayout, const DescriptorSet::Config& config)
    {
        VH_LOG_INFO("Creating Vulkan DescriptorSet Implementation");

        if (device == nullptr)
        {
            VH_LOG_ERROR("Invalid DescriptorSet configuration: Device is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (descriptorSet == VK_NULL_HANDLE)
        {
            VH_LOG_ERROR("Invalid DescriptorSet configuration: VkDescriptorSet is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (descriptorSetLayout == VK_NULL_HANDLE)
        {
            VH_LOG_ERROR("Invalid DescriptorSet configuration: VkDescriptorSetLayout is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<DescriptorSet::BindingDescription> bindingDescriptions;
        for (uint32_t i = 0; i < config.BindingCount; i++)
        {
            if (config.Bindings[i].DescriptorsCount == 0 || config.Bindings[i].StageFlags == ShaderStages::UNDEFINED || config.Bindings[i].Type == DescriptorType::UNDEFINED)
            {
                VH_LOG_ERROR("Invalid binding description at index {}: Binding, DescriptorsCount, StageFlags, and Type must be set.", i);
                return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            bindingDescriptions.PushBack(config.Bindings[i]);
        }

        return VulkanHelper::UniquePtr<Impl>(new Impl(device, descriptorSet, descriptorSetLayout, Move(bindingDescriptions)));
    }

    DescriptorSet::Impl::~Impl()
    {
        if (m_DescriptorSetLayout != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan DescriptorSet Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_DescriptorSetLayout);
            m_DescriptorSetLayout = VK_NULL_HANDLE;
            m_DescriptorSet = VK_NULL_HANDLE;
            m_Device = nullptr;
        }
    }

    DescriptorSet::Impl::Impl(DescriptorSet::Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_DescriptorSet(other.m_DescriptorSet)
        , m_DescriptorSetLayout(other.m_DescriptorSetLayout)
        , m_BindingDescriptions(Move(other.m_BindingDescriptions))
    {
        other.m_Device = nullptr;
        other.m_DescriptorSet = VK_NULL_HANDLE;
        other.m_DescriptorSetLayout = VK_NULL_HANDLE;
    }

    DescriptorSet::Impl& DescriptorSet::Impl::operator=(DescriptorSet::Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_DescriptorSet = other.m_DescriptorSet;
        other.m_DescriptorSet = VK_NULL_HANDLE;
        m_DescriptorSetLayout = other.m_DescriptorSetLayout;
        other.m_DescriptorSetLayout = VK_NULL_HANDLE;
        m_BindingDescriptions = Move(other.m_BindingDescriptions);

        return *this;
    }

    VHResult DescriptorSet::Impl::AddBuffer(uint32_t binding, uint32_t arrayIndex, const Buffer& buffer)
    {
        VH_LOG_INFO("Adding buffer to descriptor set at binding: {}, array index: {}", binding, arrayIndex);

        // Validate binding exists in the descriptor set
        bool bindingFound = false;
        DescriptorType expectedType = DescriptorType::UNDEFINED;
        uint32_t descriptorCount = 0;

        for (uint32_t i = 0; i < m_BindingDescriptions.Size(); i++)
        {
            if (m_BindingDescriptions[i].Binding == binding)
            {
                bindingFound = true;
                expectedType = m_BindingDescriptions[i].Type;
                descriptorCount = m_BindingDescriptions[i].DescriptorsCount;
                break;
            }
        }

        if (!bindingFound)
        {
            VH_LOG_ERROR("Binding {} not found in descriptor set layout", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate descriptor type
        if (expectedType != DescriptorType::UNIFORM_BUFFER && 
            expectedType != DescriptorType::STORAGE_BUFFER &&
            expectedType != DescriptorType::UNIFORM_BUFFER_DYNAMIC &&
            expectedType != DescriptorType::STORAGE_BUFFER_DYNAMIC)
        {
            VH_LOG_ERROR("Binding {} is not a buffer descriptor type", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate array index
        if (arrayIndex >= descriptorCount)
        {
            VH_LOG_ERROR("Array index {} is out of bounds for binding {} (max: {})", arrayIndex, binding, descriptorCount - 1);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Create buffer info
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = Buffer::Impl::GetImplementation(&buffer)->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = Buffer::Impl::GetImplementation(&buffer)->GetSize();

        // Update descriptor set
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = arrayIndex;
        descriptorWrite.descriptorType = static_cast<VkDescriptorType>(expectedType);
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &descriptorWrite, 0, nullptr);

        return VHResult::OK;
    }

    VHResult DescriptorSet::Impl::AddImage(uint32_t binding, uint32_t arrayIndex, const ImageView& imageView, Image::Layout layout)
    {
        VH_LOG_INFO("Adding image to descriptor set at binding: {}, array index: {}", binding, arrayIndex);

        // Validate binding exists in the descriptor set
        bool bindingFound = false;
        DescriptorType expectedType = DescriptorType::UNDEFINED;
        uint32_t descriptorCount = 0;

        for (uint32_t i = 0; i < m_BindingDescriptions.Size(); ++i)
        {
            if (m_BindingDescriptions[i].Binding == binding)
            {
                bindingFound = true;
                expectedType = m_BindingDescriptions[i].Type;
                descriptorCount = m_BindingDescriptions[i].DescriptorsCount;
                break;
            }
        }

        if (!bindingFound)
        {
            VH_LOG_ERROR("Binding {} not found in descriptor set layout", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        if (expectedType == DescriptorType::COMBINED_IMAGE_SAMPLER)
        {
            VH_LOG_ERROR("COMBINED_IMAGE_SAMPLER is not supported for now, please provide sampler separately");
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate descriptor type
        if (expectedType != DescriptorType::SAMPLED_IMAGE && 
            expectedType != DescriptorType::STORAGE_IMAGE &&
            expectedType != DescriptorType::INPUT_ATTACHMENT)
        {
            VH_LOG_ERROR("Binding {} is not an image descriptor type", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate array index
        if (arrayIndex >= descriptorCount)
        {
            VH_LOG_ERROR("Array index {} is out of bounds for binding {} (max: {})", arrayIndex, binding, descriptorCount - 1);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Create image info
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = static_cast<VkImageLayout>(layout);
        imageInfo.imageView = ImageView::Impl::GetImplementation(&imageView)->GetImageView();
        imageInfo.sampler = VK_NULL_HANDLE; // Will be set separately if needed

        // Update descriptor set
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = arrayIndex;
        descriptorWrite.descriptorType = static_cast<VkDescriptorType>(expectedType);
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &descriptorWrite, 0, nullptr);

        return VHResult::OK;
    }

    VHResult DescriptorSet::Impl::AddSampler(uint32_t binding, uint32_t arrayIndex, const Sampler& sampler)
    {
        VH_LOG_INFO("Adding sampler to descriptor set at binding: {}, array index: {}", binding, arrayIndex);

        // Validate binding exists in the descriptor set
        bool bindingFound = false;
        DescriptorType expectedType = DescriptorType::UNDEFINED;
        uint32_t descriptorCount = 0;

        for (uint32_t i = 0; i < m_BindingDescriptions.Size(); ++i)
        {
            if (m_BindingDescriptions[i].Binding == binding)
            {
                bindingFound = true;
                expectedType = m_BindingDescriptions[i].Type;
                descriptorCount = m_BindingDescriptions[i].DescriptorsCount;
                break;
            }
        }

        if (!bindingFound)
        {
            VH_LOG_ERROR("Binding {} not found in descriptor set layout", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate descriptor type
        if (expectedType != DescriptorType::SAMPLER && 
            expectedType != DescriptorType::COMBINED_IMAGE_SAMPLER)
        {
            VH_LOG_ERROR("Binding {} is not a sampler descriptor type", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate array index
        if (arrayIndex >= descriptorCount)
        {
            VH_LOG_ERROR("Array index {} is out of bounds for binding {} (max: {})", arrayIndex, binding, descriptorCount - 1);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Create image info for sampler
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = Sampler::Impl::GetImplementation(&sampler)->GetSampler();
        imageInfo.imageView = VK_NULL_HANDLE;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // Update descriptor set
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = arrayIndex;
        descriptorWrite.descriptorType = static_cast<VkDescriptorType>(expectedType);
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &descriptorWrite, 0, nullptr);

        return VHResult::OK;
    }

    VHResult DescriptorSet::Impl::AddAccelerationStructure(uint32_t binding, uint32_t arrayIndex, const TLAS::Impl* accelerationStructure)
    {
        VH_LOG_INFO("Adding acceleration structure to descriptor set at binding: {}, array index: {}", binding, arrayIndex);

        // Validate binding exists in the descriptor set
        bool bindingFound = false;
        DescriptorType expectedType = DescriptorType::UNDEFINED;
        uint32_t descriptorCount = 0;

        for (uint32_t i = 0; i < m_BindingDescriptions.Size(); ++i)
        {
            if (m_BindingDescriptions[i].Binding == binding)
            {
                bindingFound = true;
                expectedType = m_BindingDescriptions[i].Type;
                descriptorCount = m_BindingDescriptions[i].DescriptorsCount;
                break;
            }
        }

        if (!bindingFound)
        {
            VH_LOG_ERROR("Binding {} not found in descriptor set layout", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate descriptor type
        if (expectedType != DescriptorType::ACCELERATION_STRUCTURE_KHR)
        {
            VH_LOG_ERROR("Binding {} is not an acceleration structure descriptor type", binding);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Validate array index
        if (arrayIndex >= descriptorCount)
        {
            VH_LOG_ERROR("Array index {} is out of bounds for binding {} (max: {})", arrayIndex, binding, descriptorCount - 1);
            return VHResult::WRONG_ARGUMENTS;
        }

        // Create acceleration structure info
        VkWriteDescriptorSetAccelerationStructureKHR accelStructInfo{};
        accelStructInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        accelStructInfo.accelerationStructureCount = 1;
        VkAccelerationStructureKHR handle = accelerationStructure->GetHandle();
        accelStructInfo.pAccelerationStructures = &handle;

        // Update descriptor set
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = arrayIndex;
        descriptorWrite.descriptorType = static_cast<VkDescriptorType>(expectedType);
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pNext = &accelStructInfo;

        vkUpdateDescriptorSets(m_Device->GetDevice(), 1, &descriptorWrite, 0, nullptr);

        return VHResult::OK;
    }

    //
    //  Forward Functions
    //

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    DescriptorSet& DescriptorSet::operator=(DescriptorSet&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~DescriptorSet(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    DescriptorSet::~DescriptorSet()
    {

    }

    DescriptorSet::DescriptorSet(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    VHResult DescriptorSet::AddBuffer(uint32_t binding, uint32_t arrayIndex, const Buffer& buffer)
    {
        return m_Impl->AddBuffer(binding, arrayIndex, buffer);
    }

    VHResult DescriptorSet::AddImage(uint32_t binding, uint32_t arrayIndex, const ImageView& imageView, Image::Layout layout)
    {
        return m_Impl->AddImage(binding, arrayIndex, imageView, layout);
    }

    VHResult DescriptorSet::AddSampler(uint32_t binding, uint32_t arrayIndex, const Sampler& sampler)
    {
        return m_Impl->AddSampler(binding, arrayIndex, sampler);
    }

    VHResult DescriptorSet::AddAccelerationStructure(uint32_t binding, uint32_t arrayIndex, const TLAS* accelerationStructure)
    {
        return m_Impl->AddAccelerationStructure(binding, arrayIndex, TLAS::Impl::GetImplementation(accelerationStructure));
    }
} // namespace VulkanHelper