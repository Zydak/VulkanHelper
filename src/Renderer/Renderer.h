#pragma once
#include "Pch.h"

#include "Vulkan/Swapchain.h"

namespace VulkanHelper
{
	class Window;

	class Renderer
	{
	public:

		struct CreateInfo
		{
			Device* Device = nullptr;
			Window* Window = nullptr;
			VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
			uint32_t MaxFramesInFlight = 0;
		};

		Renderer(const CreateInfo& createInfo);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&& other) noexcept;
		Renderer& operator=(Renderer&& other) noexcept;

	public:

		[[nodiscard]] bool BeginFrame();
		void EndFrame();

		void BeginRendering(const std::vector<VkRenderingAttachmentInfo>& colorAttachments, VkRenderingAttachmentInfo* depthAttachment, const VkExtent2D& renderSize);
		void EndRendering();

	public:

		[[nodiscard]] inline Swapchain* GetSwapchain() const { return m_Swapchain.get(); }
		[[nodiscard]] inline VkCommandBuffer GetCurrentCommandBuffer();

		[[nodiscard]] inline uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }
		[[nodiscard]] inline uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

	private:
		void RecreateSwapchain();
		void CreateCommandBuffers();

		std::unique_ptr<Swapchain> m_Swapchain;
		Device* m_Device = nullptr;
		Window* m_Window = nullptr;

		VkExtent2D m_PreviousExtent = { 0, 0 };

		uint32_t m_MaxFramesInFlight = 0;
		uint32_t m_CurrentFrameIndex = 0;
		uint32_t m_CurrentImageIndex = 0;

		std::vector<VkCommandBuffer> m_CommandBuffers;

		bool m_IsFrameStarted = false;

		void Destroy();
		void Move(Renderer&& other) noexcept;
	};

} // namespace VulkanHelper