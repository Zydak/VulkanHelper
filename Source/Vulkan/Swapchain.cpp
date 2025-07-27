#include "Vulkan/Swapchain.h"
#include "Core/Error.h"
#include "Log/Log.h"
#include <expected>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    // TODO: REALLY beffy function, needs to be split up
    std::expected<Swapchain, VHResult> Swapchain::New(const Config& config)
    {
        if (config.Device == nullptr || config.Window == nullptr)
        {
            VH_LOG_ERROR("Invalid Swapchain configuration: Device or Window is null.");
            return std::unexpected(VHResult::WRONG_ARGUMENTS);
        }
        if (config.MaxFramesInFlight == 0)
        {
            VH_LOG_ERROR("MaxFramesInFlight must be greater than 0.");
            return std::unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Get surface capabilities
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(config.Device->GetPhysicalDevice().GetDevice(), config.Window->GetSurface(), &surfaceCapabilities);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface capabilities");
            return std::unexpected(VHResult(res));
        }

        // Get format info
        uint32_t formatCount;
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(config.Device->GetPhysicalDevice().GetDevice(), config.Window->GetSurface(), &formatCount, nullptr);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface formats");
            return std::unexpected(VHResult(res));
        }
        if (formatCount == 0)
        {
            VH_LOG_ERROR("No surface formats available!");
            return std::unexpected(VHResult::INITIALIZATION_FAILED);
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(config.Device->GetPhysicalDevice().GetDevice(), config.Window->GetSurface(), &formatCount, formats.data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface formats");
            return std::unexpected(VHResult(res));
        }

        // Get present modes
        uint32_t presentModeCount;
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(config.Device->GetPhysicalDevice().GetDevice(), config.Window->GetSurface(), &presentModeCount, nullptr);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface present modes");
            return std::unexpected(VHResult(res));
        }
        if (presentModeCount == 0)
        {
            VH_LOG_ERROR("No surface present modes available!");
            return std::unexpected(VHResult::INITIALIZATION_FAILED);
        }
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(config.Device->GetPhysicalDevice().GetDevice(), config.Window->GetSurface(), &presentModeCount, presentModes.data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface present modes");
            return std::unexpected(VHResult(res));
        }

        // Choose format
        VkSurfaceFormatKHR chosenFormat = formats[0]; // Default to first format
        for (const auto& format : formats)
        {
            if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                chosenFormat = format;
                break;
            }
        }

        // Choose present mode
        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Just use FIFO

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1; // Use one more than minimum
        if (imageCount < config.MaxFramesInFlight)
            imageCount = config.MaxFramesInFlight; // Ensure we have enough images

        if (imageCount > surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount > 0)
        {
            VH_LOG_ERROR("Requested image count exceeds maximum supported by surface! Check your MaxFramesInFlight setting, it's probably too high.");
            return std::unexpected(VHResult::INITIALIZATION_FAILED);
        }

        // Create fences and semaphores for synchronization
        std::vector<VkFence> frameFences(config.MaxFramesInFlight, VK_NULL_HANDLE);
        std::vector<VkSemaphore> acquireSemaphores(config.MaxFramesInFlight, VK_NULL_HANDLE);
        std::vector<VkSemaphore> submitSemaphores(imageCount, VK_NULL_HANDLE);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (auto& fence : frameFences)
        {
            VkResult res = vkCreateFence(config.Device->GetDevice(), &fenceInfo, nullptr, &fence);
            if (res != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create fence for swapchain");
                for (auto f : frameFences) vkDestroyFence(config.Device->GetDevice(), f, nullptr);
                return std::unexpected(VHResult(res));
            }
        }
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (auto& semaphore : acquireSemaphores)
        {
            VkResult res = vkCreateSemaphore(config.Device->GetDevice(), &semaphoreInfo, nullptr, &semaphore);
            if (res != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create acquire semaphore for swapchain");
                for (auto s : acquireSemaphores) vkDestroySemaphore(config.Device->GetDevice(), s, nullptr);
                return std::unexpected(VHResult(res));
            }
        }
        for (auto& semaphore : submitSemaphores)
        {
            VkResult res = vkCreateSemaphore(config.Device->GetDevice(), &semaphoreInfo, nullptr, &semaphore);
            if (res != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create acquire semaphore for swapchain");
                for (auto s : submitSemaphores) vkDestroySemaphore(config.Device->GetDevice(), s, nullptr);
                return std::unexpected(VHResult(res));
            }
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = config.Window->GetSurface();
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = chosenFormat.format;
        swapchainCreateInfo.imageColorSpace = chosenFormat.colorSpace;
        swapchainCreateInfo.imageExtent = { config.Window->GetWidth(), config.Window->GetHeight() };
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Single queue
        swapchainCreateInfo.queueFamilyIndexCount = 0; // No sharing
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = chosenPresentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // No previous swapchain

        VkSwapchainKHR swapchain;
        res = vkCreateSwapchainKHR(config.Device->GetDevice(), &swapchainCreateInfo, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create swapchain");
            return std::unexpected(VHResult(res));
        }

        vkGetSwapchainImagesKHR(config.Device->GetDevice(), swapchain, &imageCount, nullptr);
        VH_LOG_INFO("Swapchain created successfully with {} images", imageCount);
        std::vector<VkImage> swapchainImages(imageCount);
        res = vkGetSwapchainImagesKHR(config.Device->GetDevice(), swapchain, &imageCount, swapchainImages.data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get swapchain images");
            vkDestroySwapchainKHR(config.Device->GetDevice(), swapchain, nullptr);
            return std::unexpected(VHResult(res));
        }

        return Swapchain{
            config.Device,
            swapchain,
            config.MaxFramesInFlight,
            0, // Start at frame 0
            imageCount,
            0, // Start at image 0
            std::move(frameFences),
            std::move(acquireSemaphores),
            std::move(submitSemaphores)
        };
    }

    Swapchain::~Swapchain()
    {
        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
            VH_LOG_INFO("Destroying Vulkan Swapchain");
        }
        for (auto& fence : m_FrameFences)
        {
            if (fence != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_Device->GetDevice(), fence, nullptr);
                fence = VK_NULL_HANDLE;
            }
        }
        for (auto& semaphore : m_AcquireSemapores)
        {
            if (semaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_Device->GetDevice(), semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
            }
        }
        for (auto& semaphore : m_SubmitSemaphores)
        {
            if (semaphore != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_Device->GetDevice(), semaphore, nullptr);
                semaphore = VK_NULL_HANDLE;
            }
        }
    }

    Swapchain::Swapchain(Swapchain&& other) noexcept
        : m_Device(other.m_Device),
          m_Swapchain(other.m_Swapchain),
          m_MaxFramesInFlight(other.m_MaxFramesInFlight),
          m_CurrentFrameIndex(other.m_CurrentFrameIndex),
          m_ImageCount(other.m_ImageCount),
          m_CurrentImageIndex(other.m_CurrentImageIndex),
          m_FrameFences(std::move(other.m_FrameFences)),
          m_AcquireSemapores(std::move(other.m_AcquireSemapores)),
          m_SubmitSemaphores(std::move(other.m_SubmitSemaphores))
    {
        other.m_Device = nullptr;
        other.m_Swapchain = VK_NULL_HANDLE;
    }

    Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Device = other.m_Device;
        m_Swapchain = other.m_Swapchain;
        m_MaxFramesInFlight = other.m_MaxFramesInFlight;
        m_CurrentFrameIndex = other.m_CurrentFrameIndex;
        m_ImageCount = other.m_ImageCount;
        m_CurrentImageIndex = other.m_CurrentImageIndex;
        m_FrameFences = std::move(other.m_FrameFences);
        m_AcquireSemapores = std::move(other.m_AcquireSemapores);
        m_SubmitSemaphores = std::move(other.m_SubmitSemaphores);

        other.m_Device = nullptr;
        other.m_Swapchain = VK_NULL_HANDLE;

        return *this;
    }

    VHResult Swapchain::AcquireNextImage()
    {
        VkFence frameFence = m_FrameFences[m_CurrentFrameIndex];
        vkWaitForFences(m_Device->GetDevice(), 1, &frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device->GetDevice(), 1, &frameFence);

        VkSemaphore acquireSemaphore = m_AcquireSemapores[m_CurrentFrameIndex];

        return (VHResult)vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, UINT64_MAX, acquireSemaphore, frameFence, &m_CurrentImageIndex);
    }

    VHResult Swapchain::Submit(VkCommandBuffer commandBuffer)
    {
        VkSemaphore acquireSemaphore = m_AcquireSemapores[m_CurrentFrameIndex];
        VkSemaphore submitSemaphore = m_SubmitSemaphores[m_CurrentImageIndex];

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &acquireSemaphore;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &submitSemaphore;

        VkFence frameFence = m_FrameFences[m_CurrentFrameIndex];
        VkResult res = vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, frameFence);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to submit command buffer to queue");
            return VHResult(res);
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &submitSemaphore;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        res = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to present swapchain image");
            return VHResult(res);
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;

        return VHResult::OK;
    }
}