#pragma once
#include "Core/Vector.h"

#include "Renderer/Renderer.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/ImageView.h"
#include "Vulkan/DescriptorPool.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/glm.hpp>

#include <unordered_map>

#include <vulkan/vulkan.h>

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

        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(const SharedPtr<Device::Impl>& device, const SharedPtr<Window::Impl>& window);

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Renderer& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Renderer CreatePublicInterface(const SharedPtr<Impl>& impl) { return Renderer(impl); }

        [[nodiscard]] Expected<CommandBuffer, VHResult> BeginFrame(bool* outWasSwapchainRecreated);
        [[nodiscard]] VHResult EndFrame(bool* outWasSwapchainRecreated);

        void BeginRendering(
            const VulkanHelper::Vector<SharedPtr<ImageView::Impl>>& targetImagesColor,
            const SharedPtr<ImageView::Impl>& targetImageDepth,
            glm::vec4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f},
            float clearDepth = 1.0f,
            const SharedPtr<ImageView::Impl>& resolveImageView = nullptr,
            glm::uvec2 scissorsStart = {0u, 0u},
            glm::uvec2 scissorsEnd = {0u, 0u}
        );
        void EndRendering();

        void BeginImGuiRendering(
            glm::vec4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f}
        );

        void EndImGuiRendering();

        uint32_t CreateImGuiDescriptorSet(const SharedPtr<ImageView::Impl>& imageView, const SharedPtr<Sampler::Impl>& sampler, const Image::Layout& imageLayout);
        void RenderImGuiImage(uint32_t index, glm::vec2 size);

        [[nodiscard]] inline Image GetCurrentSwapchainImage() const { return m_Swapchain.GetCurrentSwapchainImage(); }
        [[nodiscard]] inline ImageView GetCurrentSwapchainImageView() const { return m_Swapchain.GetCurrentSwapchainImageView(); }
        [[nodiscard]] inline Format GetSwapchainImageFormat() const { return m_Swapchain.GetSwapchainImageFormat(); }
        [[nodiscard]] inline uint32_t GetSwapchainImageWidth() const { return m_Swapchain.GetSwapchainImageWidth(); }
        [[nodiscard]] inline uint32_t GetSwapchainImageHeight() const { return m_Swapchain.GetSwapchainImageHeight(); }

    private:
        VHResult RecreateSwapchain();
        
        SharedPtr<Device::Impl> m_Device;
        SharedPtr<Window::Impl> m_Window;
        Swapchain m_Swapchain;
        CommandPool m_CommandPool;
        Vector<CommandBuffer> m_CommandBuffers;
        DescriptorPool m_ImGuiDescriptorPool;
        std::unordered_map<uint32_t, VkDescriptorSet> m_ImGuiDescriptorSets;

        Impl(
            const SharedPtr<Device::Impl>& device,
            const SharedPtr<Window::Impl>& window,
            Swapchain&& swapchain,
            CommandPool&& pool,
            Vector<CommandBuffer>&& commandBuffers,
            const DescriptorPool& imguiPool
        )
            : m_Device(device)
            , m_Window(window)
            , m_Swapchain(VulkanHelper::Move(swapchain))
            , m_CommandPool(VulkanHelper::Move(pool))
            , m_CommandBuffers(VulkanHelper::Move(commandBuffers))
            , m_ImGuiDescriptorPool(imguiPool)
        {}
    };
}