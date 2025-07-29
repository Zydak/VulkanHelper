#pragma once
#include "Vulkan/Swapchain.h"

typedef struct VkSwapchainKHR_T* VkSwapchainKHR;
typedef struct VkFence_T* VkFence;
typedef struct VkSemaphore_T* VkSemaphore;

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
        [[nodiscard]] VHResult Submit(VkCommandBuffer commandBuffer);

    private:

        Impl(
            Device::Impl* device,
            VkSwapchainKHR swapchain,
            uint32_t maxFramesInFlight,
            uint32_t currentFrameIndex,
            uint32_t imageCount,
            uint32_t currentImageIndex,
            VulkanHelper::Vector<VkFence>&& frameFences,
            VulkanHelper::Vector<VkSemaphore>&& acquireSemaphores,
            VulkanHelper::Vector<VkSemaphore>&& submitSemaphores)
            : m_Device(device),
              m_Swapchain(swapchain),
              m_MaxFramesInFlight(maxFramesInFlight),
              m_CurrentFrameIndex(currentFrameIndex),
              m_ImageCount(imageCount),
              m_CurrentImageIndex(currentImageIndex),
              m_FrameFences(std::move(frameFences)),
              m_AcquireSemapores(std::move(acquireSemaphores)),
              m_SubmitSemaphores(std::move(submitSemaphores))
        {}

        VulkanHelper::Device::Impl* m_Device;
        VkSwapchainKHR m_Swapchain;

        uint32_t m_MaxFramesInFlight;
        uint32_t m_CurrentFrameIndex;
        uint32_t m_ImageCount;
        uint32_t m_CurrentImageIndex = 0;

        VulkanHelper::Vector<VkFence> m_FrameFences;
        VulkanHelper::Vector<VkSemaphore> m_AcquireSemapores;
        VulkanHelper::Vector<VkSemaphore> m_SubmitSemaphores;
    };
}