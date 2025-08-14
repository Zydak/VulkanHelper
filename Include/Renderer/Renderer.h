#pragma once

#include "Core/UniquePtr.h"
#include "Core/SharedPtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"

#include "Window/Window.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/ImageView.h"
#include "Vulkan/Sampler.h"

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
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Window to render to
             * @note Must be a valid window
             */
            VulkanHelper::Window Window{};
        };

        /**
         * @brief Creates a new renderer instance
         * @param config Configuration parameters for the renderer
         * @return Expected containing the renderer instance or error code
         */
        [[nodiscard]] static Expected<Renderer, VHResult> New(const Config& config);

        Renderer();
        ~Renderer();

        Renderer(const Renderer& other);
        Renderer& operator=(const Renderer& other);
        Renderer(Renderer&& other) noexcept;
        Renderer& operator=(Renderer&& other) noexcept;

        /**
         * @brief Begins a new frame for rendering
         * @return Expected containing command buffer or error code
         * @note Must be paired with EndFrame()
         */
        [[nodiscard]] Expected<CommandBuffer, VHResult> BeginFrame(bool* outWasSwapchainRecreated);

        /**
         * @brief Ends the current frame and submits for presentation
         * @return VHResult indicating success or failure
         */
        [[nodiscard]] VHResult EndFrame(bool* outWasSwapchainRecreated);

        /**
         * @brief Begins a render pass for specified render targets
         * @param targetImagesColor Array of color target image views
         * @param targetImageDepth Optional depth target image view
         * @param clearColor Clear color for the render targets
         * @param clearDepth Clear depth value for the depth target
         * @param scissorsStart Start coordinates of render area
         * @param scissorsEnd End coordinates of render area
         * @note Must be paired with EndRendering()
         */
        void BeginRendering(
            const VulkanHelper::Vector<ImageView>& targetImagesColor,
            const ImageView* targetImageDepth,
            glm::vec4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f},
            float clearDepth = 1.0f,
            const ImageView* resolveImageView = nullptr,
            glm::uvec2 scissorsStart = {0u, 0u},
            glm::uvec2 scissorsEnd = {0u, 0u}
        );

        /**
         * @brief Begins ImGui rendering
         * @param clearColor Clear color for the ImGui framebuffer
         */
        void BeginImGuiRendering(
            glm::vec4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f}
        );

        /**
         * @brief Ends ImGui rendering
         * @note Must be called after BeginImGuiRendering()
         */
        void EndImGuiRendering();

        /**
         * @brief Ends the current render pass
         */
        void EndRendering();

        uint32_t CreateImGuiDescriptorSet(
            const ImageView& imageView,
            const Sampler& sampler,
            const Image::Layout& imageLayout
        );

        void RenderImGuiImage(
            uint32_t index,
            glm::vec2 size
        );

        /**
         * @brief Gets the current swapchain image being rendered to
         * @return Current swapchain image
         * @note Valid only between BeginFrame() and EndFrame()
         */
        [[nodiscard]] Image GetCurrentSwapchainImage() const;

        /**
         * @brief Gets the current swapchain image view
         * @return Current swapchain image view
         * @note Valid only between BeginFrame() and EndFrame()
         */
        [[nodiscard]] ImageView GetCurrentSwapchainImageView() const;

        /**
         * @brief Gets the format of swapchain images
         * @return Format enum value of the swapchain images
         */
        [[nodiscard]] Format GetSwapchainImageFormat() const;

        /**
         * @brief Gets the width of the swapchain images
         * @return Width in pixels of the swapchain images
         */
        [[nodiscard]] uint32_t GetSwapchainImageWidth() const;

        /**
         * @brief Gets the height of the swapchain images
         * @return Height in pixels of the swapchain images
         */
        [[nodiscard]] uint32_t GetSwapchainImageHeight() const;

        class Impl;
    private:
        friend Impl;
        SharedPtr<Impl> m_Impl;

        Renderer(const SharedPtr<Impl>& impl);
    };
}