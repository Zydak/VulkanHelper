#pragma once
#include "Pch.h"

#include "Device.h"

#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
	class Swapchain
	{
	public:

		struct CreateInfo
		{
			Device* Device = nullptr;
			VkExtent2D Extent = { 0, 0 };
			VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
			uint32_t MaxFramesInFlight = 0;
			VkSurfaceKHR Surface;
			Swapchain* PreviousSwapchain = nullptr;
		};

		Swapchain(const CreateInfo& createInfo);
		~Swapchain();

		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;
		Swapchain(Swapchain&& other) noexcept;
		Swapchain& operator=(Swapchain&& other) noexcept;

	public:

		[[nodiscard]] VkResult SubmitCommandBuffer(VkCommandBuffer buffer, uint32_t& imageIndex);
		[[nodiscard]] VkResult AcquireNextImage(uint32_t& imageIndex);

		[[nodiscard]] VkFormat FindDepthFormat();

	public:

		[[nodiscard]] inline VkImageView GetPresentableImageView(int imageIndex) const { return m_PresentableImageViews[imageIndex]; }
		[[nodiscard]] inline VkImage GetPresentableImage(int imageIndex) const { return m_PresentableImages[imageIndex]; }

		[[nodiscard]] inline VkFormat GetSwapchainImageFormat() const { return m_SwapchainImageFormat; }
		[[nodiscard]] inline uint32_t GetImageCount() const { return (uint32_t)m_PresentableImageViews.size(); }
		[[nodiscard]] inline VkExtent2D GetSwapchainExtent() const { return m_Extent; }

		[[nodiscard]] inline VkPresentModeKHR GetCurrentPresentMode() const { return m_CurrentPresentMode; }

	private:

		void CreateSwapchain(Swapchain* oldSwapchain);
		void CreateImageViews();
		void CreateSyncObjects();

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

	private:

		Device* m_Device;

		VkPresentModeKHR m_CurrentPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;

		uint32_t m_MaxFramesInFlight = 0;

		VkSwapchainKHR m_Handle = VK_NULL_HANDLE;
		VkExtent2D m_Extent = { 0, 0 };
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		uint32_t m_CurrentFrame = 0;

		std::vector<VkImage> m_PresentableImages;
		std::vector<VkImageView> m_PresentableImageViews;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
		VkFormat m_SwapchainDepthFormat = VK_FORMAT_UNDEFINED;

		void Destroy();
		void Move(Swapchain&& other) noexcept;
	};

}