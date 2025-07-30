#pragma once
#include "Vulkan/Swapchain.h"
#include "Vulkan/Fence.h"
#include "Vulkan/Semaphore.h"
#include "Vulkan/Device.h"
#include "Vulkan/CommandBuffer.h"

typedef struct VkSwapchainKHR_T* VkSwapchainKHR;

namespace VulkanHelper
{
    class Swapchain::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Config& config);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] VHResult AcquireNextImage();
        [[nodiscard]] VHResult Submit(CommandBuffer& commandBuffer);

    private:

        Impl(
            Device::Impl* device,
            VkSwapchainKHR swapchain,
            uint32_t maxFramesInFlight,
            uint32_t currentFrameIndex,
            uint32_t imageCount,
            uint32_t currentImageIndex,
            VulkanHelper::Vector<Fence>&& frameFences,
            VulkanHelper::Vector<Semaphore>&& acquireSemaphores,
            VulkanHelper::Vector<Semaphore>&& submitSemaphores)
            : m_Device(device),
              m_Swapchain(swapchain),
              m_MaxFramesInFlight(maxFramesInFlight),
              m_CurrentFrameIndex(currentFrameIndex),
              m_ImageCount(imageCount),
              m_CurrentImageIndex(currentImageIndex),
              m_FrameFences(VulkanHelper::Move(frameFences)),
              m_AcquireSemaphores(VulkanHelper::Move(acquireSemaphores)),
              m_SubmitSemaphores(VulkanHelper::Move(submitSemaphores))
        {}

        VulkanHelper::Device::Impl* m_Device;
        VkSwapchainKHR m_Swapchain;

        uint32_t m_MaxFramesInFlight;
        uint32_t m_CurrentFrameIndex;
        uint32_t m_ImageCount;
        uint32_t m_CurrentImageIndex = 0;

        VulkanHelper::Vector<Fence> m_FrameFences;
        VulkanHelper::Vector<Semaphore> m_AcquireSemaphores;
        VulkanHelper::Vector<Semaphore> m_SubmitSemaphores;
    };
}