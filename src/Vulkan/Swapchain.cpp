#include "Pch.h"

#include "Swapchain.h"
#include "Logger/Logger.h"

#include "Image.h"

namespace VulkanHelper
{

	void Swapchain::Destroy()
	{
		if (m_Handle == VK_NULL_HANDLE)
			return;

		m_Device->WaitUntilIdle();

		for (auto imageView : m_PresentableImageViews) 
		{
			vkDestroyImageView(m_Device->GetHandle(), imageView, nullptr);
		}

		m_PresentableImageViews.clear();

		vkDestroySwapchainKHR(m_Device->GetHandle(), m_Handle, nullptr);
		m_Handle = VK_NULL_HANDLE;

		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			vkDestroySemaphore(m_Device->GetHandle(), m_RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(m_Device->GetHandle(), m_ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(m_Device->GetHandle(), m_InFlightFences[i], nullptr);
		}
	}

	Swapchain::Swapchain(const CreateInfo& createInfo)
	{
		m_Device = createInfo.Device;
		m_Surface = createInfo.Surface;
		m_MaxFramesInFlight = createInfo.MaxFramesInFlight;
		m_CurrentPresentMode = createInfo.PresentMode;

		m_Extent = createInfo.Extent;
		m_SwapchainDepthFormat = FindDepthFormat();

		CreateSwapchain(createInfo.PreviousSwapchain);
		CreateImageViews();
		CreateSyncObjects();
	}

	Swapchain::Swapchain(Swapchain&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));
		
		return *this;
	}

	Swapchain::~Swapchain()
	{
		Destroy();
	}

	VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			// https://stackoverflow.com/questions/12524623/what-are-the-practical-differences-when-working-with-colors-in-a-linear-vs-a-no
			if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	void Swapchain::CreateSwapchain(Swapchain* oldSwapchain)
	{
		Instance::PhysicalDevice::SwapchainSupportDetails swapChainSupport = m_Device->GetPhysicalDevice().QuerySwapchainSupport(m_Surface);

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);

		uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
		if (imageCount < m_MaxFramesInFlight)
			imageCount += m_MaxFramesInFlight - imageCount;

		VH_ASSERT(imageCount <= swapChainSupport.Capabilities.maxImageCount, "Image Count Is Too Big!");

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = m_Extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;

		createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

		createInfo.presentMode = m_CurrentPresentMode;
		createInfo.clipped = VK_TRUE;    // discards pixels that are obscured (for example behind other window)

		createInfo.oldSwapchain = oldSwapchain == nullptr ? VK_NULL_HANDLE : oldSwapchain->m_Handle;

		VH_CHECK(vkCreateSwapchainKHR(m_Device->GetHandle(), &createInfo, nullptr, &m_Handle) == VK_SUCCESS, "failed to create swap chain");

		vkGetSwapchainImagesKHR(m_Device->GetHandle(), m_Handle, &imageCount, nullptr);
		m_PresentableImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_Device->GetHandle(), m_Handle, &imageCount, m_PresentableImages.data());

		for (int i = 0; i < m_PresentableImages.size(); i++)
		{
			Image::TransitionImageLayout(
				m_Device,
				m_PresentableImages[i],
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0
			);

			m_Device->SetObjectName(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_PresentableImages[i], "Swapchain Image");
		}

		m_SwapchainImageFormat = surfaceFormat.format;
	}

	void Swapchain::CreateImageViews()
	{
		m_PresentableImageViews.resize(m_PresentableImages.size());

		for (uint32_t i = 0; i < m_PresentableImages.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_PresentableImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = m_SwapchainImageFormat;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			VH_CHECK(vkCreateImageView(m_Device->GetHandle(), &createInfo, nullptr, &m_PresentableImageViews[i]) == VK_SUCCESS,
				"failed to create texture image view"
			);
		}
	}

	VkFormat Swapchain::FindDepthFormat()
	{
		return m_Device->GetPhysicalDevice().FindSupportedFormat({ VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	VkResult Swapchain::SubmitCommandBuffer(VkCommandBuffer buffer, uint32_t& imageIndex)
	{
		if (m_ImagesInFlight[imageIndex] != VK_NULL_HANDLE) 
		{
			vkWaitForFences(m_Device->GetHandle(), 1, &m_ImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		m_ImagesInFlight[imageIndex] = m_InFlightFences[m_CurrentFrame];

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores = m_ImageAvailableSemaphores[m_CurrentFrame];
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;

		VkSemaphore signalSemaphores = m_RenderFinishedSemaphores[m_CurrentFrame];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphores;

		m_Device->GetGraphicsQueueMutex()->lock();
		vkResetFences(m_Device->GetHandle(), 1, &m_InFlightFences[m_CurrentFrame]);
     		VH_CHECK(vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) == VK_SUCCESS,
			"failed to submit draw command buffer!"
			);
		m_Device->GetGraphicsQueueMutex()->unlock();

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_Handle };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

		m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFramesInFlight;

		return result;
	}

	VkResult Swapchain::AcquireNextImage(uint32_t& imageIndex)
	{
		vkWaitForFences(m_Device->GetHandle(), 1, &m_InFlightFences[(m_CurrentFrame + 1) % m_MaxFramesInFlight], VK_TRUE, UINT64_MAX);

		VkResult result = vkAcquireNextImageKHR(m_Device->GetHandle(), m_Handle, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

		return result;
	}

	void Swapchain::CreateSyncObjects()
	{
		m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
		m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
		m_InFlightFences.resize(m_MaxFramesInFlight);
		m_ImagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < m_MaxFramesInFlight; i++)
		{
			if (vkCreateSemaphore(m_Device->GetHandle(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_Device->GetHandle(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_Device->GetHandle(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
			{
				VH_CHECK(false, "failed to create synchronization objects!");
			}
		}
	}

	void Swapchain::Move(Swapchain&& other) noexcept
	{
		m_Device = other.m_Device;
		other.m_Device = nullptr;

		m_Surface = other.m_Surface;
		other.m_Surface = VK_NULL_HANDLE;

		m_CurrentPresentMode = other.m_CurrentPresentMode;
		other.m_CurrentPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;

		m_MaxFramesInFlight = other.m_MaxFramesInFlight;
		other.m_MaxFramesInFlight = 0;

		m_Handle = other.m_Handle;
		other.m_Handle = VK_NULL_HANDLE;

		m_Extent = other.m_Extent;
		other.m_Extent = { 0, 0 };

		m_CurrentFrame = other.m_CurrentFrame;
		other.m_CurrentFrame = 0;

		m_PresentableImages = other.m_PresentableImages;
		other.m_PresentableImages.clear();

		m_PresentableImageViews = other.m_PresentableImageViews;
		other.m_PresentableImageViews.clear();

		m_ImageAvailableSemaphores = other.m_ImageAvailableSemaphores;
		other.m_ImageAvailableSemaphores.clear();

		m_RenderFinishedSemaphores = other.m_RenderFinishedSemaphores;
		other.m_RenderFinishedSemaphores.clear();

		m_InFlightFences = other.m_InFlightFences;
		other.m_InFlightFences.clear();

		m_ImagesInFlight = other.m_ImagesInFlight;
		other.m_ImagesInFlight.clear();

		m_SwapchainImageFormat = other.m_SwapchainImageFormat;
		other.m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;

		m_SwapchainDepthFormat = other.m_SwapchainDepthFormat;
		other.m_SwapchainDepthFormat = VK_FORMAT_UNDEFINED;
	}

} // namespace VulkanHelper