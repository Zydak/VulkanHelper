#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"

#include "Window/Window.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/ImageView.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/glm.hpp>

namespace VulkanHelper
{
    /**
     * @class Renderer
     * @brief High-level rendering interface for swapchain presentation and command submission
     */
    class Renderer
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a renderer
         */
        struct Config
        {
            /**
             * @brief Vulkan logical device for rendering
             * @note Must not be nullptr and must outlive the renderer
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief Window to render to
             * @note Must not be nullptr and must outlive the renderer
             */
            VulkanHelper::Window* Window = nullptr;

            /**
             * @brief Number of frames that can be rendered simultaneously
             * @note Must be at least 1. Use 2 for double buffering, 3 for triple buffering
             */
            uint32_t FramesInFlight = 1;
        };

        /**
         * @brief Creates a new renderer instance
         * @param config Configuration parameters for the renderer
         * @return Expected containing the renderer instance or error code
         */
        [[nodiscard]] static Expected<Renderer, VHResult> New(const Config& config);

        /**
         * @brief Destructor
         */
        ~Renderer();

        /**
         * @brief Delete copy constructor
         */
        Renderer(const Renderer& other) = delete;

        /**
         * @brief Delete copy assignment operator
         */
        Renderer& operator=(const Renderer& other) = delete;

        /**
         * @brief Move constructor
         */
        Renderer(Renderer&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        Renderer& operator=(Renderer&& other) noexcept;

        /**
         * @brief Begins a new frame for rendering
         * @return Expected containing command buffer or error code
         * @note Must be paired with EndFrame()
         */
        [[nodiscard]] Expected<CommandBuffer*, VHResult> BeginFrame();

        /**
         * @brief Ends the current frame and submits for presentation
         * @return VHResult indicating success or failure
         */
        [[nodiscard]] VHResult EndFrame();

        /**
         * @brief Begins a render pass for specified render targets
         * @param commandBuffer Command buffer to record commands into
         * @param targetImagesColor Array of color target image views
         * @param targetImageDepth Optional depth target image view
         * @param clearColor Clear color for the render targets
         * @param clearDepth Clear depth value for the depth target
         * @param scissorsStart Start coordinates of render area
         * @param scissorsEnd End coordinates of render area
         * @note Must be paired with EndRendering()
         */
        void BeginRendering(
            CommandBuffer& commandBuffer,
            const VulkanHelper::Vector<ImageView*>& targetImagesColor,
            const ImageView* targetImageDepth,
            glm::vec4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f},
            float clearDepth = 1.0f,
            glm::uvec2 scissorsStart = {0u, 0u},
            glm::uvec2 scissorsEnd = {0u, 0u}
        );

        /**
         * @brief Ends the current render pass
         * @param commandBuffer Command buffer containing the render pass
         */
        void EndRendering(CommandBuffer& commandBuffer);

        /**
         * @brief Gets the current swapchain image being rendered to
         * @return Pointer to the current swapchain image
         * @note Valid only between BeginFrame() and EndFrame()
         */
        [[nodiscard]] Image* GetCurrentSwapchainImage() const;

        /**
         * @brief Gets the current swapchain image view
         * @return Pointer to the current swapchain image view
         * @note Valid only between BeginFrame() and EndFrame()
         */
        [[nodiscard]] ImageView* GetCurrentSwapchainImageView() const;

        /**
         * @brief Gets the format of swapchain images
         * @return Format enum value of the swapchain images
         */
        [[nodiscard]] Format GetSwapchainImageFormat() const;

        class Impl;
    private:
        friend Impl;
        UniquePtr<Impl> m_Impl;

        Renderer(UniquePtr<Impl>&& impl);
    };
}