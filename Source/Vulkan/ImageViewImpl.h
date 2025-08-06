#pragma once
#include "Vulkan/ImageView.h"

#include "ImageImpl.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class ImageView::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Image::Impl* image, ImageView::ViewType viewType, uint32_t baseLayer, uint32_t layerCount);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const ImageView* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static ImageView CreatePublicInterface(VulkanHelper::UniquePtr<Impl>&& impl) { return ImageView(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline const Image::Impl* GetImage() const { return m_Image; }
        [[nodiscard]] inline VkImageView GetImageView() const { return m_ImageView; }
        [[nodiscard]] inline ViewType GetViewType() const { return m_ViewType; }

        [[nodiscard]] inline Format GetFormat() const { return m_Image->GetFormat(); }
        [[nodiscard]] inline Image::Layout GetLayout() const { return m_Image->GetLayout(); }
        [[nodiscard]] inline Image::Aspect GetAspect() const { return m_Image->GetAspect(); }
        
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Image->GetWidth(); }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Image->GetHeight(); }
        [[nodiscard]] inline uint32_t GetLayerCount() const { return m_Image->GetLayerCount(); }
        [[nodiscard]] inline uint32_t GetMipCount() const { return m_Image->GetMipCount(); }

    private:
        Impl(const Image::Impl* image, VkImageView view, ViewType type)
            : m_Image(image)
            , m_ImageView(view)
            , m_ViewType(type)
        {}

        const Image::Impl* m_Image;
        VkImageView m_ImageView;
        ViewType m_ViewType;
    };
}