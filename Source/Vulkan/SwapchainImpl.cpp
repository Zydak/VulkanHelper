#include "Core/Enums.h"
#include "Vulkan/PhysicalDevice.h"
#include "Vulkan/Swapchain.h"
#include "SwapchainImpl.h"

#include "Core/Error.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "DeviceImpl.h"
#include "PhysicalDeviceImpl.h"
#include "FenceImpl.h"
#include "SemaphoreImpl.h"
#include "ImageImpl.h"
#include "Window/Window.h"

#include "CommandPoolImpl.h"
#include "CommandBufferImpl.h"

namespace VulkanHelper
{
    // TODO: REALLY beffy function, needs to be split up
    Expected<SharedPtr<Swapchain::Impl>, VHResult> Swapchain::Impl::New(
        const SharedPtr<Device::Impl>& device,
        const SharedPtr<Window::Impl>& window,
        SharedPtr<Swapchain::Impl> previousSwapchain
    )
    {
        SharedPtr<PhysicalDevice::Impl> physicalDeviceImpl = device->GetPhysicalDevice();

        // Get surface capabilities
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDeviceImpl->GetDevice(), window->GetSurface(), &surfaceCapabilities);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface capabilities");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Get format info
        uint32_t formatCount;
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceImpl->GetDevice(), window->GetSurface(), &formatCount, nullptr);
        if (formatCount == 0 || res != VK_SUCCESS)
        {
            VH_LOG_ERROR("No surface formats available!");
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
        }
        VulkanHelper::Vector<VkSurfaceFormatKHR> formats(formatCount);
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceImpl->GetDevice(), window->GetSurface(), &formatCount, formats.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface formats");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Get present modes
        uint32_t presentModeCount;
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceImpl->GetDevice(), window->GetSurface(), &presentModeCount, nullptr);
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
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceImpl->GetDevice(), window->GetSurface(), &presentModeCount, presentModes.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface present modes");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Choose format
        VkSurfaceFormatKHR chosenFormat = formats[0]; // Default to first format
        for (size_t i = 0; i < formats.Size(); i++)
        {
            // Prefer UNORM layouts over SRGB
            if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
            {
                chosenFormat = formats[i];
                break;
            }
            else if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                chosenFormat = formats[i];
            }
        }

        // Choose present mode
        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Just use FIFO

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1; // Use one more than minimum
        if (imageCount > surfaceCapabilities.maxImageCount && surfaceCapabilities.maxImageCount > 0)
        {
            VH_LOG_ERROR("Requested image count exceeds maximum supported by surface! Check your MaxFramesInFlight setting, it's probably too high.");
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        // Create fences and semaphores for synchronization
        VulkanHelper::Vector<Semaphore> acquireSemaphores;
        VulkanHelper::Vector<Semaphore> submitSemaphores;

        auto fenceResult = Fence::Impl::New(device, true);
        if (!fenceResult.HasValue())
        {
            VH_LOG_ERROR("Failed to create fence for swapchain implementation");
            return VulkanHelper::Unexpected(fenceResult.Error());
        }
        Fence frameFence = Fence::Impl::CreatePublicInterface(VulkanHelper::Move(fenceResult.Value()));

        for (size_t i = 0; i < 2; i++)
        {
            auto semaphore = Semaphore::Impl::New(device);
            if (!semaphore.HasValue())
            {
                VH_LOG_ERROR("Failed to create semaphore for swapchain implementation");
                return VulkanHelper::Unexpected(semaphore.Error());
            }
            acquireSemaphores.PushBack(Semaphore::Impl::CreatePublicInterface(VulkanHelper::Move(semaphore.Value())));
        }

        for (size_t i = 0; i < imageCount; i++)
        {
            auto semaphore = Semaphore::Impl::New(device);
            if (!semaphore.HasValue())
            {
                VH_LOG_ERROR("Failed to create semaphore for swapchain implementation");
                return VulkanHelper::Unexpected(semaphore.Error());
            }
            submitSemaphores.PushBack(Semaphore::Impl::CreatePublicInterface(VulkanHelper::Move(semaphore.Value())));
        }

        VkSwapchainKHR prevSwapchain = VK_NULL_HANDLE;
        if (previousSwapchain != nullptr)
        {
            prevSwapchain = previousSwapchain->GetSwapchain();
        }

        // Create swapchain
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = window->GetSurface();
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = chosenFormat.format;
        swapchainCreateInfo.imageColorSpace = chosenFormat.colorSpace;
        swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // Single queue
        swapchainCreateInfo.queueFamilyIndexCount = 0; // No sharing
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = chosenPresentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = prevSwapchain;

        VkSwapchainKHR swapchain;
        res = vkCreateSwapchainKHR(device->GetDevice(), &swapchainCreateInfo, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create swapchain implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &imageCount, nullptr);
        VH_LOG_INFO("Swapchain Implementation created successfully with {} images", imageCount);
        VulkanHelper::Vector<VkImage> swapchainImages(imageCount);
        res = vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &imageCount, swapchainImages.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get swapchain images");
            vkDestroySwapchainKHR(device->GetDevice(), swapchain, nullptr);
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Buffer for layout transition
        SharedPtr<CommandPool::Impl> commandPool = CommandPool::Impl::New(device, CommandPool::Flags::TRANSIENT_BIT, device->GetQueueFamilyIndices().GraphicsFamily).Value();
        CommandBuffer commandBuffer = commandPool->AllocateCommandBuffer(commandPool, CommandBuffer::Level::PRIMARY).Value();
        VH_ASSERT(commandBuffer.BeginRecording(CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Failed to begin command buffer recording");

        VulkanHelper::Vector<Image> images;
        VulkanHelper::Vector<ImageView> imageViews;
        for (size_t i = 0; i < swapchainImages.Size(); i++)
        {
            SharedPtr<Image::Impl> imageImpl( new Image::Impl(
                device,
                (Format)chosenFormat.format,
                {Image::Layout::UNDEFINED},
                Image::Aspect::COLOR_BIT,
                surfaceCapabilities.currentExtent.width,
                surfaceCapabilities.currentExtent.height,
                1,
                1,
                swapchainImages[i]
            ));

            Image image(VulkanHelper::Move(imageImpl));
            image.TransitionImageLayout(Image::Layout::PRESENT_SRC_KHR, commandBuffer);
            images.PushBack(VulkanHelper::Move(image));

            auto imageView = ImageView::New({images[i], ImageView::ViewType::VIEW_2D});
            if (!imageView.HasValue())
            {
                VH_LOG_ERROR("Failed to create image views");
                vkDestroySwapchainKHR(device->GetDevice(), swapchain, nullptr);
                return VulkanHelper::Unexpected(imageView.Error());
            }

            imageViews.PushBack(Move(imageView.Value()));
        }
        VH_ASSERT(commandBuffer.EndRecording() == VulkanHelper::VHResult::OK, "Failed to end command buffer recording");
        VH_ASSERT(commandBuffer.SubmitAndWait() == VulkanHelper::VHResult::OK, "Failed to submit command buffer");

        return SharedPtr( new Swapchain::Impl(
            device,
            swapchain,
            0, // Start at frame 0
            imageCount,
            0, // Start at image 0
            VulkanHelper::Move(images),
            VulkanHelper::Move(imageViews),
            VulkanHelper::Move(frameFence),
            VulkanHelper::Move(acquireSemaphores),
            VulkanHelper::Move(submitSemaphores)
        ));
    }

    Swapchain::Impl::~Impl()
    {
        if (m_Swapchain != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Vulkan Swapchain Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Swapchain);
            m_Swapchain = VK_NULL_HANDLE;
        }
    }

    Swapchain::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device),
          m_Swapchain(other.m_Swapchain),
          m_CurrentFrameIndex(other.m_CurrentFrameIndex),
          m_ImageCount(other.m_ImageCount),
          m_CurrentImageIndex(other.m_CurrentImageIndex),
          m_Images(VulkanHelper::Move(other.m_Images)),
          m_ImageViews(VulkanHelper::Move(other.m_ImageViews)),
          m_FrameFence(VulkanHelper::Move(other.m_FrameFence)),
          m_AcquireSemaphores(VulkanHelper::Move(other.m_AcquireSemaphores)),
          m_SubmitSemaphores(VulkanHelper::Move(other.m_SubmitSemaphores))
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
        m_CurrentFrameIndex = other.m_CurrentFrameIndex;
        m_ImageCount = other.m_ImageCount;
        m_CurrentImageIndex = other.m_CurrentImageIndex;
        m_Images = VulkanHelper::Move(other.m_Images);
        m_ImageViews = VulkanHelper::Move(other.m_ImageViews);
        m_FrameFence = VulkanHelper::Move(other.m_FrameFence);
        m_AcquireSemaphores = VulkanHelper::Move(other.m_AcquireSemaphores);
        m_SubmitSemaphores = VulkanHelper::Move(other.m_SubmitSemaphores);

        other.m_Device = nullptr;
        other.m_Swapchain = VK_NULL_HANDLE;

        return *this;
    }

    VHResult Swapchain::Impl::AcquireNextImage()
    {
        SharedPtr<Semaphore::Impl> acquireSemaphoreImpl = Semaphore::Impl::GetImplementation(m_AcquireSemaphores[m_CurrentFrameIndex]);
        VkSemaphore acquireSemaphore = acquireSemaphoreImpl->GetSemaphore();
        
        return (VHResult)vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &m_CurrentImageIndex);
    }

    VHResult Swapchain::Impl::Submit(const SharedPtr<CommandBuffer::Impl>& commandBuffer)
    {
        SharedPtr<Semaphore::Impl> submitSemaphoreImpl = Semaphore::Impl::GetImplementation(m_SubmitSemaphores[m_CurrentImageIndex]);
        VkSemaphore submitSemaphoreVk = submitSemaphoreImpl->GetSemaphore();

        VkFence fence = Fence::Impl::GetImplementation(m_FrameFence)->GetFenceHandle();
        vkWaitForFences(m_Device->GetDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device->GetDevice(), 1, &fence);
        VHResult res = commandBuffer->Submit(
            PipelineStages::COLOR_ATTACHMENT_OUTPUT_BIT,
            {Semaphore::Impl::GetImplementation(m_AcquireSemaphores[m_CurrentFrameIndex])},
            {Semaphore::Impl::GetImplementation(m_SubmitSemaphores[m_CurrentImageIndex])},
            Fence::Impl::GetImplementation(m_FrameFence)
        );

        if (res != VHResult::OK)
        {
            return res;
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &submitSemaphoreVk;

        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        VkQueue presentQueue;
        vkGetDeviceQueue(m_Device->GetDevice(), m_Device->GetQueueFamilyIndices().PresentFamily, 0, &presentQueue);
        res = (VHResult)vkQueuePresentKHR(presentQueue, &presentInfo);
        if (res != VHResult::OK)
        {
            return res;
        }

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % 2;

        return VHResult::OK;
    }

    //
    // Forward functions
    //
    
    VulkanHelper::Expected<Swapchain, VHResult> Swapchain::New(const Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Swapchain Implementation");

        SharedPtr<Swapchain::Impl> previousSwapchainImpl = nullptr;
        if (config.PreviousSwapchain != nullptr)
            previousSwapchainImpl = Swapchain::Impl::GetImplementation(*config.PreviousSwapchain);
        
        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            Window::Impl::GetImplementation(config.Window),
            previousSwapchainImpl
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Swapchain::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    Swapchain::Swapchain()
        : m_Impl(nullptr)
    {
        
    }

    Swapchain::~Swapchain()
    {

    }

    Swapchain::Swapchain(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }

    Swapchain::Swapchain(const Swapchain& other)
        : m_Impl(other.m_Impl)
    {
        
    }

    Swapchain& Swapchain::operator=(const Swapchain& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;

        return *this;
    }

    Swapchain::Swapchain(Swapchain&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Swapchain(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    VHResult Swapchain::AcquireNextImage()
    {
        return m_Impl->AcquireNextImage();
    }

    VHResult Swapchain::Submit(const CommandBuffer& commandBuffer)
    {
        return m_Impl->Submit(CommandBuffer::Impl::GetImplementation(commandBuffer));
    }

    Image Swapchain::GetCurrentSwapchainImage() const
    {
        return m_Impl->GetCurrentSwapchainImage();
    }

    ImageView Swapchain::GetCurrentSwapchainImageView() const
    {
        return m_Impl->GetCurrentSwapchainImageView();
    }

    uint32_t Swapchain::GetCurrentFrameIndex() const
    {
        return m_Impl->GetCurrentFrameIndex();
    }

    Format Swapchain::GetSwapchainImageFormat() const
    {
        return m_Impl->GetSwapchainImageFormat();
    }

    uint32_t Swapchain::GetSwapchainImageWidth() const
    {
        return m_Impl->GetImageWidth();
    }

    uint32_t Swapchain::GetSwapchainImageHeight() const
    {
        return m_Impl->GetImageHeight();
    }
}