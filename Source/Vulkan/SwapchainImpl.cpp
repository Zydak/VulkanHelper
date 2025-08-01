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

namespace VulkanHelper
{
    // TODO: REALLY beffy function, needs to be split up
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Swapchain::Impl>, VHResult> Swapchain::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Creating Vulkan Swapchain Implementation");

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

        PhysicalDevice::Impl* physicalDeviceImpl = PhysicalDevice::Impl::GetImplementation(&config.Device->GetPhysicalDevice());

        // Get surface capabilities
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDeviceImpl->GetDevice(), config.Window->GetSurface(), &surfaceCapabilities);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface capabilities");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Get format info
        uint32_t formatCount;
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceImpl->GetDevice(), config.Window->GetSurface(), &formatCount, nullptr);
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
        res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceImpl->GetDevice(), config.Window->GetSurface(), &formatCount, formats.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get surface formats");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        // Get present modes
        uint32_t presentModeCount;
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceImpl->GetDevice(), config.Window->GetSurface(), &presentModeCount, nullptr);
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
        res = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceImpl->GetDevice(), config.Window->GetSurface(), &presentModeCount, presentModes.Data());
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
        VulkanHelper::Vector<Fence> frameFences;
        VulkanHelper::Vector<Semaphore> acquireSemaphores;
        VulkanHelper::Vector<Semaphore> submitSemaphores;

        for (size_t i = 0; i < config.MaxFramesInFlight; i++)
        {
            auto fence = Fence::New({config.Device, true});
            if (!fence.HasValue())
            {
                VH_LOG_ERROR("Failed to create fence for swapchain implementation");
                return VulkanHelper::Unexpected(fence.Error());
            }
            frameFences.PushBack(VulkanHelper::Move(fence.Value()));
        }

        for (size_t i = 0; i < config.MaxFramesInFlight; i++)
        {
            auto semaphore = Semaphore::New({config.Device});
            if (!semaphore.HasValue())
            {
                VH_LOG_ERROR("Failed to create semaphore for swapchain implementation");
                return VulkanHelper::Unexpected(semaphore.Error());
            }
            acquireSemaphores.PushBack(VulkanHelper::Move(semaphore.Value()));
        }

        for (size_t i = 0; i < imageCount; i++)
        {
            auto semaphore = Semaphore::New({config.Device});
            if (!semaphore.HasValue())
            {
                VH_LOG_ERROR("Failed to create semaphore for swapchain implementation");
                return VulkanHelper::Unexpected(semaphore.Error());
            }
            submitSemaphores.PushBack(VulkanHelper::Move(semaphore.Value()));
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

        Device::Impl* deviceImpl = Device::Impl::GetImplementation(config.Device);
        VkSwapchainKHR swapchain;
        res = vkCreateSwapchainKHR(deviceImpl->GetDevice(), &swapchainCreateInfo, nullptr, &swapchain);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create swapchain implementation");
            return VulkanHelper::Unexpected(VHResult(res));
        }

        vkGetSwapchainImagesKHR(deviceImpl->GetDevice(), swapchain, &imageCount, nullptr);
        VH_LOG_INFO("Swapchain Implementation created successfully with {} images", imageCount);
        VulkanHelper::Vector<VkImage> swapchainImages(imageCount);
        res = vkGetSwapchainImagesKHR(deviceImpl->GetDevice(), swapchain, &imageCount, swapchainImages.Data());
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to get swapchain images");
            vkDestroySwapchainKHR(deviceImpl->GetDevice(), swapchain, nullptr);
            return VulkanHelper::Unexpected(VHResult(res));
        }

        VulkanHelper::Vector<Image> images;
        VulkanHelper::Vector<ImageView> imageViews;
        for (size_t i = 0; i < swapchainImages.Size(); i++)
        {
            UniquePtr<Image::Impl> imageImpl( new Image::Impl(
                deviceImpl,
                (Format)chosenFormat.format,
                Image::Layout::UNDEFINED,
                MemoryProperties::UNDEFINED,
                Image::Aspect::COLOR_BIT,
                config.Window->GetWidth(),
                config.Window->GetHeight(),
                1,
                1,
                swapchainImages[i]
            ));

            Image image(VulkanHelper::Move(imageImpl));
            images.PushBack(VulkanHelper::Move(image));

            auto imageView = ImageView::New({&images[i], ImageView::ViewType::VIEW_2D});
            if (!imageView.HasValue())
            {
                VH_LOG_ERROR("Failed to create image views");
                vkDestroySwapchainKHR(deviceImpl->GetDevice(), swapchain, nullptr);
                return VulkanHelper::Unexpected(imageView.Error());
            }

            imageViews.PushBack(Move(imageView.Value()));
        }

        return VulkanHelper::UniquePtr(new Swapchain::Impl{
            deviceImpl,
            swapchain,
            config.MaxFramesInFlight,
            0, // Start at frame 0
            imageCount,
            0, // Start at image 0
            VulkanHelper::Move(images),
            VulkanHelper::Move(imageViews),
            VulkanHelper::Move(frameFences),
            VulkanHelper::Move(acquireSemaphores),
            VulkanHelper::Move(submitSemaphores)
        });
    }

    Swapchain::Impl::~Impl()
    {
        if (m_Swapchain != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying Vulkan Swapchain Implementation");
            vkDestroySwapchainKHR(m_Device->GetDevice(), m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }
    }

    Swapchain::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device),
          m_Swapchain(other.m_Swapchain),
          m_FramesInFlight(other.m_FramesInFlight),
          m_CurrentFrameIndex(other.m_CurrentFrameIndex),
          m_ImageCount(other.m_ImageCount),
          m_CurrentImageIndex(other.m_CurrentImageIndex),
          m_FrameFences(VulkanHelper::Move(other.m_FrameFences)),
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
        m_FramesInFlight = other.m_FramesInFlight;
        m_CurrentFrameIndex = other.m_CurrentFrameIndex;
        m_ImageCount = other.m_ImageCount;
        m_CurrentImageIndex = other.m_CurrentImageIndex;
        m_FrameFences = VulkanHelper::Move(other.m_FrameFences);
        m_AcquireSemaphores = VulkanHelper::Move(other.m_AcquireSemaphores);
        m_SubmitSemaphores = VulkanHelper::Move(other.m_SubmitSemaphores);

        other.m_Device = nullptr;
        other.m_Swapchain = VK_NULL_HANDLE;

        return *this;
    }

    VHResult Swapchain::Impl::AcquireNextImage()
    {
        Fence::Impl* frameFenceImpl = Fence::Impl::GetImplementation(&m_FrameFences[m_CurrentFrameIndex]);
        VkFence frameFence = frameFenceImpl->GetFenceHandle();
        vkWaitForFences(m_Device->GetDevice(), 1, &frameFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_Device->GetDevice(), 1, &frameFence);

        Semaphore::Impl* acquireSemaphoreImpl = Semaphore::Impl::GetImplementation(&m_AcquireSemaphores[m_CurrentFrameIndex]);
        VkSemaphore acquireSemaphore = acquireSemaphoreImpl->GetSemaphore();

        return (VHResult)vkAcquireNextImageKHR(m_Device->GetDevice(), m_Swapchain, UINT64_MAX, acquireSemaphore, nullptr, &m_CurrentImageIndex);
    }

    VHResult Swapchain::Impl::Submit(CommandBuffer& commandBuffer)
    {
        Semaphore* acquireSemaphore = &m_AcquireSemaphores[m_CurrentFrameIndex];
        Semaphore* submitSemaphore = &m_SubmitSemaphores[m_CurrentImageIndex];
        Semaphore::Impl* submitSemaphoreImpl = Semaphore::Impl::GetImplementation(&m_SubmitSemaphores[m_CurrentImageIndex]);
        VkSemaphore submitSemaphoreVk = submitSemaphoreImpl->GetSemaphore();

        VHResult res = commandBuffer.Submit(PipelineStages::COLOR_ATTACHMENT_OUTPUT_BIT, acquireSemaphore, submitSemaphore, &m_FrameFences[m_CurrentFrameIndex]);
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

        m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_FramesInFlight;

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

        return Swapchain{ VulkanHelper::Move(implResult.Value()) };
    }

    Swapchain::~Swapchain()
    {

    }

    Swapchain::Swapchain(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
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

    VHResult Swapchain::Submit(CommandBuffer& commandBuffer)
    {
        return m_Impl->Submit(commandBuffer);
    }

    Image* Swapchain::GetCurrentSwapchainImage() const
    {
        return m_Impl->GetCurrentSwapchainImage();
    }

    ImageView* Swapchain::GetCurrentSwapchainImageView() const
    {
        return m_Impl->GetCurrentSwapchainImageView();
    }

    uint32_t Swapchain::GetCurrentFrameIndex() const
    {
        return m_Impl->GetCurrentFrameIndex();
    }

    uint32_t Swapchain::GetFramesInFlightCount() const
    {
        return m_Impl->GetFramesInFlight();
    }
}