#include "Vulkan/DescriptorPool.h"
#include "DescriptorPoolImpl.h"
#include "DescriptorSetImpl.h"

#include "Log/Log.h"
#include "Vulkan/Device.h"
#include "Core/Error.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    Expected<SharedPtr<DescriptorPool::Impl>, VHResult> DescriptorPool::Impl::New(
        const SharedPtr<Device::Impl>& device,
        uint32_t maxSets,
        const PoolSize* poolSizes,
        uint32_t poolSizeCount,
        Flags poolFlags
    )
    {
        VH_LOG_INFO("Creating Vulkan DescriptorPool Implementation");

        if (device == nullptr)
        {
            VH_LOG_ERROR("Invalid DescriptorPool configuration: Device is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (poolSizes == nullptr || poolSizeCount == 0)
        {
            VH_LOG_ERROR("Invalid DescriptorPool configuration: PoolSizes is null or PoolSizeCount is zero.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (maxSets == 0)
        {
            VH_LOG_ERROR("Invalid DescriptorPool configuration: MaxSets cannot be zero.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Convert PoolSize to VkDescriptorPoolSize
        VulkanHelper::Vector<VkDescriptorPoolSize> vkPoolSizes;
        vkPoolSizes.Reserve(poolSizeCount);

        for (uint32_t i = 0; i < poolSizeCount; ++i)
        {
            if (poolSizes[i].DescriptorCount == 0)
            {
                VH_LOG_ERROR("Invalid DescriptorPool configuration: PoolSize[{}] has zero DescriptorCount.", i);
                return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
            }

            VkDescriptorPoolSize vkPoolSize{};
            vkPoolSize.type = static_cast<VkDescriptorType>(poolSizes[i].Type);
            vkPoolSize.descriptorCount = poolSizes[i].DescriptorCount;
            vkPoolSizes.PushBack(vkPoolSize);
        }

        // Create descriptor pool
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = static_cast<VkDescriptorPoolCreateFlags>(poolFlags);
        poolInfo.maxSets = maxSets;
        poolInfo.poolSizeCount = poolSizeCount;
        poolInfo.pPoolSizes = vkPoolSizes.Data();

        VkDescriptorPool descriptorPool;
        VkResult res = vkCreateDescriptorPool(device->GetDevice(), &poolInfo, nullptr, &descriptorPool);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create descriptor pool implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(device, descriptorPool)));
    }

    DescriptorPool::Impl::~Impl()
    {
        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan DescriptorPool Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_DescriptorPool);
            m_DescriptorPool = VK_NULL_HANDLE;
            m_Device = nullptr;
        }
    }

    DescriptorPool::Impl::Impl(DescriptorPool::Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_DescriptorPool(other.m_DescriptorPool)
    {
        other.m_Device = nullptr;
        other.m_DescriptorPool = VK_NULL_HANDLE;
    }

    DescriptorPool::Impl& DescriptorPool::Impl::operator=(DescriptorPool::Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (m_DescriptorPool != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan DescriptorPool Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_DescriptorPool);
            m_DescriptorPool = VK_NULL_HANDLE;
            m_Device = nullptr;
        }

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_DescriptorPool = other.m_DescriptorPool;
        other.m_DescriptorPool = VK_NULL_HANDLE;

        return *this;
    }

    VulkanHelper::Expected<DescriptorSet, VHResult> DescriptorPool::Impl::AllocateDescriptorSet(const DescriptorSet::Config& config)
    {
        VH_LOG_INFO("Allocating Descriptor Set from pool");

        if (config.Bindings == nullptr || config.BindingCount == 0)
        {
            VH_LOG_ERROR("Invalid DescriptorSet configuration: Bindings is null or BindingCount is zero.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Validate bindings
        for (uint32_t i = 0; i < config.BindingCount; ++i)
        {
            if (config.Bindings[i].Type == DescriptorType::UNDEFINED)
            {
                VH_LOG_ERROR("Invalid DescriptorSet configuration: Binding[{}] has UNDEFINED type.", i);
                return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
            }

            if (config.Bindings[i].StageFlags == ShaderStages::UNDEFINED)
            {
                VH_LOG_ERROR("Invalid DescriptorSet configuration: Binding[{}] has UNDEFINED stage flags.", i);
                return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
            }

            if (config.Bindings[i].DescriptorsCount == 0)
            {
                VH_LOG_ERROR("Invalid DescriptorSet configuration: Binding[{}] has zero DescriptorsCount.", i);
                return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
            }
        }

        // Create descriptor set layout
        VulkanHelper::Vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.Reserve(config.BindingCount);

        for (uint32_t i = 0; i < config.BindingCount; ++i)
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding = config.Bindings[i].Binding;
            layoutBinding.descriptorType = static_cast<VkDescriptorType>(config.Bindings[i].Type);
            layoutBinding.descriptorCount = config.Bindings[i].DescriptorsCount;
            layoutBinding.stageFlags = static_cast<VkShaderStageFlags>(config.Bindings[i].StageFlags);
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBindings.PushBack(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = config.BindingCount;
        layoutInfo.pBindings = layoutBindings.Data();

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
        VulkanHelper::Vector<VkDescriptorBindingFlags> bindingFlags;
        bindingFlags.Reserve(config.BindingCount);
        for (uint32_t i = 0; i < config.BindingCount; ++i)
        {
            VkDescriptorBindingFlags flags = 0;
            flags |= VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;
            flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

            bindingFlags.PushBack(flags);
        }

        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        bindingFlagsInfo.bindingCount = config.BindingCount;
        bindingFlagsInfo.pBindingFlags = bindingFlags.Data();
        layoutInfo.pNext = &bindingFlagsInfo;

        VkDescriptorSetLayout descriptorSetLayout;
        VkResult res = vkCreateDescriptorSetLayout(m_Device->GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create descriptor set layout, error code: {}", (int)res);
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        VkDescriptorSet descriptorSet;
        res = vkAllocateDescriptorSets(m_Device->GetDevice(), &allocInfo, &descriptorSet);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to allocate descriptor set, error code: {}", (int)res);
            vkDestroyDescriptorSetLayout(m_Device->GetDevice(), descriptorSetLayout, nullptr);
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Create DescriptorSet implementation
        auto descriptorSetImpl = DescriptorSet::Impl::New(m_Device, descriptorSet, descriptorSetLayout, config);
        if (!descriptorSetImpl.HasValue())
        {
            vkDestroyDescriptorSetLayout(m_Device->GetDevice(), descriptorSetLayout, nullptr);
            return VulkanHelper::Unexpected(descriptorSetImpl.Error());
        }

        return DescriptorSet::Impl::CreatePublicInterface(Move(descriptorSetImpl.Value()));
    }

    void DescriptorPool::Impl::Reset()
    {
        vkResetDescriptorPool(m_Device->GetDevice(), m_DescriptorPool, 0);
    }

    //
    //  Forward Functions
    //

    VulkanHelper::Expected<DescriptorPool, VHResult> DescriptorPool::New(const Config& config)
    {
        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.MaxSets,
            config.PoolSizes,
            config.PoolSizeCount,
            config.PoolFlags
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return DescriptorPool::Impl::CreatePublicInterface(Move(implResult.Value()));
    }

    DescriptorPool::DescriptorPool()
        : m_Impl(nullptr)
    {
    }

    DescriptorPool::DescriptorPool(const DescriptorPool& other)
        : m_Impl(other.m_Impl)
    {

    }

    DescriptorPool& DescriptorPool::operator=(const DescriptorPool& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;

        return *this;
    }

    DescriptorPool::DescriptorPool(DescriptorPool&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    DescriptorPool::~DescriptorPool()
    {

    }

    DescriptorPool::DescriptorPool(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }

    VulkanHelper::Expected<DescriptorSet, VHResult> DescriptorPool::AllocateDescriptorSet(const DescriptorSet::Config& config)
    {
        return m_Impl->AllocateDescriptorSet(config);
    }

    void DescriptorPool::Reset()
    {
        m_Impl->Reset();
    }
} // namespace VulkanHelper