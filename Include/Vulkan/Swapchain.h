#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"

#include "Device.h"
#include "Image.h"
#include "ImageView.h"
#include "Window/Window.h"

namespace VulkanHelper
{
    /**
     * @class Swapchain
     * @brief RAII wrapper for a Vulkan swapchain. Manages presentation of rendered images to the window surface.
     */
    class Swapchain
    {
    public:
        /**
         * @brief Configuration for swapchain creation
         */
        struct Config
        {
            /**
             * @brief The logical device that will own this swapchain
             * 
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief The window where images will be presented
             */
            VulkanHelper::Window* Window = nullptr;

            /**
             * @brief Number of frames that can be rendered simultaneously
             */
            uint32_t MaxFramesInFlight = 2;
        };

        /**
         * @brief Creates a new swapchain for window presentation.
         * 
         * @param config Swapchain creation configuration
         * @return Expected<Swapchain, VHResult> The created swapchain or error code
         * @note Device must support presentation and Window must be valid
         */
        [[nodiscard]] static VulkanHelper::Expected<Swapchain, VHResult> New(const Config& config);

        ~Swapchain();
        Swapchain(const Swapchain& other) = delete;
        Swapchain& operator=(const Swapchain& other) = delete;
        Swapchain(Swapchain&& other) noexcept;
        Swapchain& operator=(Swapchain&& other) noexcept;

        /**
         * @brief Gets the next available swapchain image for rendering.
         * 
         * @return VHResult Success or swapchain recreation required
         * @note Call this before rendering each frame
         */
        [[nodiscard]] VHResult AcquireNextImage();

        /**
         * @brief Submits rendered commands and presents the current image.
         * 
         * @param commandBuffer Commands to execute before presentation
         * @return VHResult Success or swapchain recreation required
         * @note Must be called after rendering is complete
         */
        [[nodiscard]] VHResult Submit(CommandBuffer& commandBuffer);

        /**
         * @brief Gets the current swapchain image for rendering.
         */
        [[nodiscard]] Image* GetCurrentSwapchainImage() const;

        /**
         * @brief Gets the view of the current swapchain image.
         */
        [[nodiscard]] ImageView* GetCurrentSwapchainImageView() const;

        /**
         * @brief Gets the index of the current frame being rendered.
         */
        [[nodiscard]] uint32_t GetCurrentFrameIndex() const;

        /**
         * @brief Gets the maximum number of frames that can be processed simultaneously.
         */
        [[nodiscard]] uint32_t GetFramesInFlightCount() const;

        /**
         * @brief Gets the image format used by the swapchain.
         */
        [[nodiscard]] Format GetSwapchainImageFormat() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Swapchain(VulkanHelper::UniquePtr<Impl>&& impl);
    };
}