#include "ImageViewImpl.h"

#include "ImageImpl.h"
#include "DeviceImpl.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<ImageView::Impl>, VHResult> ImageView::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Creating Image View Implementation");

        if (config.image == nullptr)
        {
            VH_LOG_ERROR("Image can't be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }
        if (config.ViewType == ViewType::VIEW_UNDEFINED)
        {
            VH_LOG_ERROR("View type can't be undefined!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        Image::Impl* imageImpl = Image::Impl::GetImplementation(config.image);
        Device::Impl* deviceImpl = imageImpl->GetDevice();

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = imageImpl->GetImage();
        viewInfo.viewType = (VkImageViewType)config.ViewType;
        viewInfo.format = (VkFormat)imageImpl->GetFormat();
        viewInfo.subresourceRange.aspectMask = (VkImageAspectFlags)imageImpl->GetAspect();
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = imageImpl->GetMipCount();
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = config.LayerCount == UINT32_MAX ? imageImpl->GetLayerCount() : config.LayerCount;

        VkImageView imageView;
        if (vkCreateImageView(deviceImpl->GetDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            VH_LOG_ERROR("Failed to create image view implementation!");
            return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        return UniquePtr(new Impl(imageImpl, imageView, config.ViewType));
    }

    ImageView::Impl::Impl(Impl&& other) noexcept
        : m_Image(other.m_Image), m_ImageView(other.m_ImageView), m_ViewType(other.m_ViewType)
    {
        other.m_Image = nullptr;
        other.m_ImageView = nullptr;
    }

    ImageView::Impl& ImageView::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // cleanup current state

        m_Image = other.m_Image;
        other.m_Image = nullptr;
        m_ImageView = other.m_ImageView;
        other.m_ImageView = nullptr;
        m_ViewType = other.m_ViewType;
        other.m_ViewType = ViewType::VIEW_UNDEFINED;

        return *this;
    }

    ImageView::Impl::~Impl()
    {
        if (m_ImageView != nullptr)
        {
            VH_LOG_INFO("Destroying ImageView Implementation");
            Device::Impl* deviceImpl = m_Image->GetDevice();
            vkDestroyImageView(deviceImpl->GetDevice(), m_ImageView, nullptr);
        }
    }

    //
    //  Forward Functions
    //

    VulkanHelper::Expected<ImageView, VHResult> ImageView::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return ImageView{ VulkanHelper::Move(implResult.Value()) };
    }

    ImageView::ImageView(ImageView&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {
        other.m_Impl = nullptr;
    }

    ImageView& ImageView::operator=(ImageView&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~ImageView(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    ImageView::~ImageView()
    {

    }

    ImageView::ImageView(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    [[nodiscard]] Format ImageView::GetFormat() const { return m_Impl->GetFormat(); }
    [[nodiscard]] Image::Layout ImageView::GetLayout() const { return m_Impl->GetLayout(); }
    [[nodiscard]] MemoryProperties ImageView::GetMemoryProperties() const { return m_Impl->GetMemoryProperties(); }
    [[nodiscard]] Image::Aspect ImageView::GetAspect() const { return m_Impl->GetAspect(); }
        
    [[nodiscard]] uint32_t ImageView::GetWidth() const { return m_Impl->GetWidth(); }
    [[nodiscard]] uint32_t ImageView::GetHeight() const { return m_Impl->GetHeight(); }
    [[nodiscard]] uint32_t ImageView::GetLayerCount() const { return m_Impl->GetLayerCount(); }
    [[nodiscard]] uint32_t ImageView::GetMipCount() const { return m_Impl->GetMipCount(); }
}