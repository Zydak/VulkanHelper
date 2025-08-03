#pragma once
#include "Core/Vector.h"

#include "Renderer/Renderer.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/ImageView.h"
#include <glm/glm.hpp>

namespace VulkanHelper
{
    class Renderer::Impl
    {
    public:
        ~Impl();

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const Renderer* publicInterface) { return publicInterface->m_Impl.Get(); }

        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(const Config& config);
        
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

        [[nodiscard]] inline Image* GetCurrentSwapchainImage() const { return m_Swapchain.GetCurrentSwapchainImage(); }
        [[nodiscard]] inline ImageView* GetCurrentSwapchainImageView() const { return m_Swapchain.GetCurrentSwapchainImageView(); }
        [[nodiscard]] inline Format GetSwapchainImageFormat() const { return m_Swapchain.GetSwapchainImageFormat(); }

    private:
        Device* m_Device;
        Window* m_Window;
        Swapchain m_Swapchain;
        CommandPool m_CommandPool;
        Vector<CommandBuffer> m_CommandBuffers;

        Impl(
            Device* device,
            Window* window,
            Swapchain&& swapchain,
            CommandPool&& pool,
            Vector<CommandBuffer>&& commandBuffers
        )
            : m_Device(device)
            , m_Window(window)
            , m_Swapchain(VulkanHelper::Move(swapchain))
            , m_CommandPool(VulkanHelper::Move(pool))
            , m_CommandBuffers(VulkanHelper::Move(commandBuffers))
        {}
    };
}