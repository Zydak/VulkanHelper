#include "Vulkan/Swapchain.h"
#include "SwapchainImpl.h"

#include "Core/Error.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>

#include "DeviceImpl.h"
#include "PhysicalDeviceImpl.h"

namespace VulkanHelper
{
    // TODO: REALLY beffy function, needs to be split up
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Swapchain::Impl>, VHResult> Swapchain::Impl::New(const Config& config)
    {
        if (config.Device == nullptr || config.Window == nullptr)
        {
            VH_LOG_ERROR("Invalid Swapchain configuration: Device or Window is null.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }
        if (config.MaxFramesInFlight == 0)
        {
            VH_LOG_ERROR("Invalid Swapchain configuration: MaxFramesInFlight must be greater than 0.");
            return VulkanHelper::Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        // Get surface capabilities
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(config.Device->GetPhysicalDevice().m_Impl->GetDevice(), config.Window->GetSurface(), &surfaceCapabilities);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface capabilities");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Get format info
        uint32_t formatCount;
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(config.Device->GetPhysicalDevice().m_Impl->GetDevice(), config.Window->GetSurface(), &formatCount, nullptr);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface formats");
            return VulkanHelper::Unexpected(VHResult(res));
        }
        if (formatCount == 0)
        {
            VH_LOG_ERROR("No surface formats available!");
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
        }
        VulkanHelper::Vector<VkSurfaceFormatKHR> formats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(config.Device->GetPhysicalDevice().m_Impl->GetDevice(), config.Window->GetSurface(), &formatCount, formats.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface formats");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Get present modes
        uint32_t presentModeCount;
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(config.Device->GetPhysicalDevice().m_Impl->GetDevice(), config.Window->GetSurface(), &presentModeCount, nullptr);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface present modes");
            return VulkanHelper::Unexpected(VHResult(res));
        }
        if (presentModeCount == 0)
        {
            VH_LOG_ERROR("No surface present modes available!");
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
        }
        VulkanHelper::Vector<VkPresentModeKHR> presentModes(presentModeCount);
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(config.Device->GetPhysicalDevice().m_Impl->GetDevice(), config.Window->GetSurface(), &presentModeCount, presentModes.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface present modes");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Choose format
        VkSurfaceFormatKHR chosenFormat = formats[0]; // Default to first format
        for (size_t i = 0; i < formats.Size(); i++)
        {
            if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                chosenFormat = formats[i];
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
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        // Create fences and semaphores for synchronization
        VulkanHelper::Vector<VkFence> frameFences(config.MaxFramesInFlight, VK_NULL_HANDLE);
        VulkanHelper::Vector<VkSemaphore> acquireSemaphores(config.MaxFramesInFlight, VK_NULL_HANDLE);
        VulkanHelper::Vector<VkSemaphore> submitSemaphores(imageCount, VK_NULL_HANDLE);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (size_t i = 0; i < frameFences.Size(); i++)
        {
            VkResult res = vkCreateFence(config.Device->m_Impl->GetDevice(), &fenceInfo, nullptr, &frameFences[i]);
            if (res != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create fence for swapchain implementation");
                for (size_t i = 0; i < frameFences.Size(); i++) vkDestroyFence(config.Device->m_Impl->GetDevice(), frameFences[i], nullptr);
                return VulkanHelper::Unexpected(VHResult(res));
            }
        }
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (size_t i = 0; i < acquireSemaphores.Size(); i++)
        {
            VkResult res = vkCreateSemaphore(config.Device->m_Impl->GetDevice(), &semaphoreInfo, nullptr, &acquireSemaphores[i]);
            if (res != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create acquire semaphore for swapchain implementation");
                for (size_t i = 0; i < acquireSemaphores.Size(); i++) vkDestroySemaphore(config.Device->m_Impl->GetDevice(), acquireSemaphores[i], nullptr);
                return VulkanHelper::Unexpected(VHResult(res));
            }
        }
        for (size_t i = 0; i < submitSemaphores.Size(); i++)
        {
            VkResult res = vkCreateSemaphore(config.Device->m_Impl->GetDevice(), &semaphoreInfo, nullptr, &submitSemaphores[i]);
            if (res != VK_SUCCESS)
            {
                VH_LOG_ERROR("Failed to create acquire semaphore for swapchain implementation");
                for (size_t i = 0; i < submitSemaphores.Size(); i++) vkDestroySemaphore(config.Device->m_Impl->GetDevice(), submitSemaphores[i], nullptr);
                return VulkanHelper::Unexpected(VHResult(res));
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
        res = vkCreateSwapchainKHR(config.Device->m_Impl->GetDevice(), &swapchainCreateInfo, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create swapchain implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        vkGetSwapchainImagesKHR(config.Device->m_Impl->GetDevice(), swapchain, &imageCount, nullptr);
        VH_LOG_INFO("Swapchain Implementation created successfully with {} images", imageCount);
        VulkanHelper::Vector<VkImage> swapchainImages(imageCount);
        res = vkGetSwapchainImagesKHR(config.Device->m_Impl->GetDevice(), swapchain, &imageCount, swapchainImages.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get swapchain images");
            vkDestroySwapchainKHR(config.Device->m_Impl->GetDevice(), swapchain, nullptr);
            return VulkanHelper::Unexpected(VHResult(res));
        }

        return VulkanHelper::UniquePtr(new Swapchain::Impl{
            config.Device->m_Impl.Get(),
            swapchain,
            config.MaxFramesInFlight,
            0, // Start at frame 0
            imageCount,
            0, // Start at image 0
            std::move(frameFences),
            std::move(acquireSemaphores),
            std::move(submitSemaphores)
        });
    }

    Swapchain::Impl::~Impl()
    {
        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
            VH_LOG_INFO("Destroying Vulkan Swapchain Implementation");
        }
        for (size_t i = 0; i < m_FrameFences.Size(); i++)
        {
            if (m_FrameFences[i] != VK_NULL_HANDLE)
            {
                vkDestroyFence(m_Device->GetDevice(), m_FrameFences[i] , nullptr);
                m_FrameFences[i]  = VK_NULL_HANDLE;
            }
        }
        for (size_t i = 0; i < m_AcquireSemapores.Size(); i++)
        {
            if (m_AcquireSemapores[i]  != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_Device->GetDevice(), m_AcquireSemapores[i] , nullptr);
                m_AcquireSemapores[i]  = VK_NULL_HANDLE;
            }
        }
        for (size_t i = 0; i < m_SubmitSemaphores.Size(); i++)
        {
            if (m_SubmitSemaphores[i]  != VK_NULL_HANDLE)
            {
                vkDestroySemaphore(m_Device->GetDevice(), m_SubmitSemaphores[i] , nullptr);
                m_SubmitSemaphores[i]  = VK_NULL_HANDLE;
            }
        }
    }

    Swapchain::Impl::Impl(Impl&& other) noexcept
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

    Swapchain::Impl& Swapchain::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

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

    VHResult Swapchain::Impl::AcquireNextImage()
    {
        VkFence frameFence = m_FrameFences[m_CurrentFrameIndex];
        vkWaitForFences(m_Device->GetDevice(), 1, &frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device->GetDevice(), 1, &frameFence);

        VkSemaphore acquireSemaphore = m_AcquireSemapores[m_CurrentFrameIndex];

        return (VHResult)vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, UINT64_MAX, acquireSemaphore, frameFence, &m_CurrentImageIndex);
    }

    VHResult Swapchain::Impl::Submit(VkCommandBuffer commandBuffer)
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
        VkQueue graphicsQueue;
        vkGetDeviceQueue(m_Device->GetDevice(), m_Device->GetQueueFamilyIndices().GraphicsFamily, 0, &graphicsQueue);
        VkResult res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, frameFence);
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

        VkQueue presentQueue;
        vkGetDeviceQueue(m_Device->GetDevice(), m_Device->GetQueueFamilyIndices().PresentFamily, 0, &presentQueue);
        res = vkQueuePresentKHR(presentQueue, &presentInfo);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to present swapchain image");
            return VHResult(res);
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;

        return VHResult::OK;
    }

    //
    // Forward functions
    //
    
    VulkanHelper::Expected<Swapchain, VHResult> Swapchain::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Swapchain{ std::move(implResult.Value()) };
    }

    Swapchain::~Swapchain()
    {
        VH_LOG_INFO("Destroying Vulkan Swapchain");
    }

    Swapchain::Swapchain(Swapchain&& other) noexcept
        : m_Impl(std::move(other.m_Impl))
    {}

    Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Swapchain(); // Clean up current state

        m_Impl = std::move(other.m_Impl);

        return *this;
    }

    VHResult Swapchain::AcquireNextImage()
    {
        return m_Impl->AcquireNextImage();
    }

    VHResult Swapchain::Submit(VkCommandBuffer commandBuffer)
    {
        return m_Impl->Submit(commandBuffer);
    }
}