#pragma once
#include "Vulkan/Swapchain.h"
#include "Vulkan/Fence.h"
#include "Vulkan/Semaphore.h"
#include "Vulkan/Image.h"
#include "Vulkan/ImageView.h"
#include "Vulkan/Device.h"
#include "Vulkan/CommandBuffer.h"

#include "DeviceImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Swapchain::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(Device::Impl* device, Window::Impl* window, Swapchain::Impl* previousSwapchain);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Swapchain* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Swapchain CreatePublicInterface(UniquePtr<Impl>&& impl) { return Swapchain(VulkanHelper::Move(impl)); }

        [[nodiscard]] VHResult AcquireNextImage();
        [[nodiscard]] VHResult Submit(CommandBuffer& commandBuffer);
        [[nodiscard]] inline Image* GetCurrentSwapchainImage() { return &m_Images[m_CurrentImageIndex]; };
        [[nodiscard]] inline ImageView* GetCurrentSwapchainImageView() { return &m_ImageViews[m_CurrentImageIndex]; };

        [[nodiscard]] inline uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

        [[nodiscard]] inline Format GetSwapchainImageFormat() const { return m_Images[0].GetFormat(); }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }

        [[nodiscard]] inline VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }

        [[nodiscard]] inline VulkanHelper::Vector<Image>& GetImages() { return m_Images; }
        [[nodiscard]] inline VulkanHelper::Vector<ImageView>& GetImageViews() { return m_ImageViews; }

        [[nodiscard]] inline uint32_t GetImageWidth() const { return m_Images[0].GetWidth(); }
        [[nodiscard]] inline uint32_t GetImageHeight() const { return m_Images[0].GetHeight(); }

    private:

        Impl(
            Device::Impl* device,
            VkSwapchainKHR swapchain,
            uint32_t currentFrameIndex,
            uint32_t imageCount,
            uint32_t currentImageIndex,
            VulkanHelper::Vector<Image>&& images,
            VulkanHelper::Vector<ImageView>&& imageViews,
            Fence&& frameFence,
            VulkanHelper::Vector<Semaphore>&& acquireSemaphores,
            VulkanHelper::Vector<Semaphore>&& submitSemaphores
        )
            : m_Device(device),
              m_Swapchain(swapchain),
              m_CurrentFrameIndex(currentFrameIndex),
              m_ImageCount(imageCount),
              m_CurrentImageIndex(currentImageIndex),
              m_Images(VulkanHelper::Move(images)),
              m_ImageViews(VulkanHelper::Move(imageViews)),
              m_FrameFence(VulkanHelper::Move(frameFence)),
              m_AcquireSemaphores(VulkanHelper::Move(acquireSemaphores)),
              m_SubmitSemaphores(VulkanHelper::Move(submitSemaphores))
        {}

        VulkanHelper::Device::Impl* m_Device;
        VkSwapchainKHR m_Swapchain;

        uint32_t m_CurrentFrameIndex;
        uint32_t m_ImageCount;
        uint32_t m_CurrentImageIndex;

        VulkanHelper::Vector<Image> m_Images;
        VulkanHelper::Vector<ImageView> m_ImageViews;

        Fence m_FrameFence;
        VulkanHelper::Vector<Semaphore> m_AcquireSemaphores;
        VulkanHelper::Vector<Semaphore> m_SubmitSemaphores;
    };
}