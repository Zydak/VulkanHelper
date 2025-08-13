#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/SharedPtr.h"

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
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief The window where images will be presented
             * @note Must be a valid window
             */
            VulkanHelper::Window Window{};

            /**
             * @brief The previous swapchain to be reused (Optional)
             */
            VulkanHelper::Swapchain* PreviousSwapchain = nullptr;
        };

        /**
         * @brief Creates a new swapchain for window presentation.
         * 
         * @param config Swapchain creation configuration
         * @return Expected<Swapchain, VHResult> The created swapchain or error code
         * @note Device must support presentation and Window must be valid
         */
        [[nodiscard]] static VulkanHelper::Expected<Swapchain, VHResult> New(const Config& config);

        Swapchain();

        Swapchain(const Swapchain& other);
        Swapchain& operator=(const Swapchain& other);

        Swapchain(Swapchain&& other) noexcept;
        Swapchain& operator=(Swapchain&& other) noexcept;

        ~Swapchain();

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
        [[nodiscard]] VHResult Submit(const CommandBuffer& commandBuffer);

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
         * @brief Gets the image format used by the swapchain.
         */
        [[nodiscard]] Format GetSwapchainImageFormat() const;

        /**
         * @brief Gets the width of the swapchain images.
         */
        [[nodiscard]] uint32_t GetSwapchainImageWidth() const;

        /**
         * @brief Gets the height of the swapchain images.
         */
        [[nodiscard]] uint32_t GetSwapchainImageHeight() const;

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        Swapchain(const SharedPtr<Impl>& impl);
    };
}