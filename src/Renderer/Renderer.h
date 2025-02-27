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

		void Init(const CreateInfo& createInfo);
		void Destroy();

		Renderer() = default;
		Renderer(const CreateInfo& createInfo);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&& other) noexcept;
		Renderer& operator=(Renderer&& other) noexcept;

		[[nodiscard]] inline bool IsInitialized() const { return m_Initialized; }
		[[nodiscard]] inline VkCommandBuffer GetCurrentCommandBuffer();

		[[nodiscard]] bool BeginFrame();
		void EndFrame();

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

		bool m_Initialized = false;
		void Move(Renderer&& other) noexcept;
		void Reset();
	};

} // namespace VulkanHelper