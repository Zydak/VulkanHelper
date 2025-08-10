#pragma once
#include "Core/Vector.h"

#include "Renderer/Renderer.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/ImageView.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
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

        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(Device::Impl* device, Window::Impl* window);

        [[nodiscard]] inline static Impl* GetImplementation(const Renderer* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Renderer CreatePublicInterface(UniquePtr<Impl>&& impl) { return Renderer(VulkanHelper::Move(impl)); }

        [[nodiscard]] Expected<CommandBuffer*, VHResult> BeginFrame(bool* outWasSwapchainRecreated);
        [[nodiscard]] VHResult EndFrame(bool* outWasSwapchainRecreated);

        void BeginRendering(
            CommandBuffer& commandBuffer,
            const VulkanHelper::Vector<ImageView*>& targetImagesColor,
            const ImageView* targetImageDepth,
            glm::vec4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f},
            float clearDepth = 1.0f,
            const ImageView* resolveImageView = nullptr,
            glm::uvec2 scissorsStart = {0u, 0u},
            glm::uvec2 scissorsEnd = {0u, 0u}
        );
        void EndRendering(CommandBuffer& commandBuffer);

        [[nodiscard]] inline Image* GetCurrentSwapchainImage() const { return m_Swapchain.GetCurrentSwapchainImage(); }
        [[nodiscard]] inline ImageView* GetCurrentSwapchainImageView() const { return m_Swapchain.GetCurrentSwapchainImageView(); }
        [[nodiscard]] inline Format GetSwapchainImageFormat() const { return m_Swapchain.GetSwapchainImageFormat(); }
        [[nodiscard]] inline uint32_t GetSwapchainImageWidth() const { return m_Swapchain.GetSwapchainImageWidth(); }
        [[nodiscard]] inline uint32_t GetSwapchainImageHeight() const { return m_Swapchain.GetSwapchainImageHeight(); }

    private:
        VHResult RecreateSwapchain();
        
        Device::Impl* m_Device;
        Window::Impl* m_Window;
        Swapchain m_Swapchain;
        CommandPool m_CommandPool;
        Vector<CommandBuffer> m_CommandBuffers;

        Impl(
            Device::Impl* device,
            Window::Impl* window,
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