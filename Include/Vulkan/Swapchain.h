#pragma once

#include "Core/Error.h"
#include "Device.h"
#include "Window/Window.h"

typedef struct VkSwapchainKHR_T* VkSwapchainKHR;
typedef struct VkFence_T* VkFence;
typedef struct VkSemaphore_T* VkSemaphore;
typedef struct VkCommandBuffer_T* VkCommandBuffer;

namespace VulkanHelper
{
    class Swapchain
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a Swapchain instance.
         *
         * Specify the logical device, window, maximum frames in flight, and swapchain dimensions.
         * Pass this struct to Swapchain::New() to create a swapchain.
         */
        struct Config
        {
            /**
             * @brief Pointer to the logical Vulkan device to use for swapchain creation.
             *
             * @note Cannot be nullptr; Also must be valid for the entire lifetime of the Swapchain.
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief Pointer to the window for which the swapchain will present images.
             *
             * @note Cannot be null.
             */
            VulkanHelper::Window* Window = nullptr;

            /**
             * @brief Maximum number of frames that can be processed concurrently (frames in flight).
             *
             * @note Must be greater than 0.
             */
            uint32_t MaxFramesInFlight = 2;
        };

        /**
         * @brief Creates a new Swapchain instance with the specified configuration.
         *
         * This static factory function attempts to create a Swapchain according to the provided configuration.
         * If successful, it returns a Swapchain object; otherwise, it returns a VHError describing the failure.
         *
         * @param config The configuration struct specifying swapchain creation options.
         * @return VulkanHelper::Expected<Swapchain, VHError> An expected containing the created Swapchain on success, or a VHError on failure.
         */
        [[nodiscard]] static VulkanHelper::Expected<Swapchain, VHResult> New(const Config& config);

        ~Swapchain();
        Swapchain(const Swapchain& other) = delete;
        Swapchain& operator=(const Swapchain& other) = delete;
        Swapchain(Swapchain&& other) noexcept;
        Swapchain& operator=(Swapchain&& other) noexcept;

        [[nodiscard]] VHResult AcquireNextImage();
        [[nodiscard]] VHResult Submit(VkCommandBuffer commandBuffer);

    private:

        Swapchain(
            Device* device,
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

        VulkanHelper::Device* m_Device;
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