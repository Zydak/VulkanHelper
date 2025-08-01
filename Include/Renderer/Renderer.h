#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"

#include "Window/Window.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Vulkan/ImageView.h"

#include <glm/glm.hpp>

namespace VulkanHelper
{
    class Renderer
    {
    public:
        struct Config
        {
            VulkanHelper::Device* Device = nullptr;
            VulkanHelper::Window* Window = nullptr;
            uint32_t FramesInFlight = 1; // Set to 2 for double buffering, 3 for tripple buffering, and so on.
        };

        [[nodiscard]] static Expected<Renderer, VHResult> New(const Config& config);

        ~Renderer();

        Renderer(const Renderer& other) = delete;
        Renderer& operator=(const Renderer& other) = delete;

        Renderer(Renderer&& other) noexcept;
        Renderer& operator=(Renderer&& other) noexcept;

        [[nodiscard]] Expected<CommandBuffer*, VHResult> BeginFrame();
        [[nodiscard]] VHResult EndFrame();

        void BeginRendering(
            CommandBuffer& commandBuffer,
            const VulkanHelper::Vector<ImageView*>& targetImagesColor,
            const ImageView* targetImageDepth,
            glm::uvec2 scissorsStart = {0u, 0u},
            glm::uvec2 scissorsEnd = {0u, 0u}
        );
        void EndRendering(CommandBuffer& commandBuffer);

        [[nodiscard]] Image* GetCurrentSwapchainImage() const;
        [[nodiscard]] ImageView* GetCurrentSwapchainImageView() const;

        class Impl;
    private:
        friend Impl;
        UniquePtr<Impl> m_Impl;

        Renderer(UniquePtr<Impl>&& impl);
    };
}