#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"
#include "Core/Macros.h"

namespace VulkanHelper
{
    class Device;
    class Window;
    class CommandBuffer;
    class Image;

    /*
     * @class Swapchain
     * @brief RAII wrapper for a Vulkan swapchain.
     *
     * Manages the lifetime of a Vulkan swapchain, including image acquisition and submission.
     * Provides functionality to create a swapchain, acquire the next image, and submit command buffers.
    */
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

        /**
        * @brief Acquires the next available image from the swapchain for rendering.
        *
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult AcquireNextImage();

        /**
        * @brief Submits a command buffer for execution and presentation to the swapchain.
        *
        * @param commandBuffer The command buffer to submit for execution and presentation.
        * @return VHResult indicating success or failure.
        */
        [[nodiscard]] VHResult Submit(CommandBuffer& commandBuffer);

        [[nodiscard]] Image* GetCurrentSwapchainImage() const;

    private:
        class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Swapchain(VulkanHelper::UniquePtr<Impl>&& impl);

        #undef SWAPCHAIN_CLASS
        DECLARE_FRIENDS();
        #define SWAPCHAIN_CLASS Swapchain
    };
}