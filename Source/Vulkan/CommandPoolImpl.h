#pragma once
#include "Vulkan/CommandPool.h"
#include "Vulkan/Device.h"
#include "DeviceImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class CommandPool::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(const SharedPtr<Device::Impl>& device, Flags flags, uint32_t queueFamilyIndex);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const CommandPool& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static CommandPool CreatePublicInterface(const SharedPtr<Impl>& impl) { return CommandPool(impl); }

        [[nodiscard]] static VulkanHelper::Expected<CommandBuffer, VHResult> AllocateCommandBuffer(const SharedPtr<Impl>& impl, CommandBuffer::Level level);

        [[nodiscard]] inline SharedPtr<Device::Impl> GetDevice() const { return m_Device; }
        [[nodiscard]] inline VkCommandPool GetCommandPool() const { return m_CommandPool; }
        [[nodiscard]] inline VkQueue GetQueue() const { return m_Queue; }
        [[nodiscard]] inline Flags GetFlags() const { return m_Flags; }
        [[nodiscard]] inline uint32_t GetQueueFamilyIndex() const { return m_QueueFamilyIndex; }
        
    private:
        Impl(const SharedPtr<Device::Impl>& device, VkCommandPool commandPool, VkQueue queue, Flags flags, uint32_t queueFamilyIndex)
        : m_Device(device), m_CommandPool(commandPool), m_Queue(queue), m_Flags(flags), m_QueueFamilyIndex(queueFamilyIndex)
        {}

        [[nodiscard]] VHResult FreeCommandBuffer(CommandBuffer::Impl* commandBuffer) const;

        SharedPtr<Device::Impl> m_Device;
        VkCommandPool m_CommandPool;
        VkQueue m_Queue;
        Flags m_Flags;
        uint32_t m_QueueFamilyIndex;

        friend class CommandBuffer::Impl; // Allow access to FreeCommandBuffer
    };
}