#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/UniquePtr.h"

#include "Vulkan/Image.h"

namespace VulkanHelper
{
    class ImageView
    {
    public:
        enum class ViewType
        {
            VIEW_1D = 0,
            VIEW_2D = 1,
            VIEW_3D = 2,
            VIEW_CUBE = 3,
            VIEW_1D_ARRAY = 4,
            VIEW_2D_ARRAY = 5,
            VIEW_CUBE_ARRAY = 6,
            VIEW_UNDEFINED = 0x7FFFFFFF
        };
        
        struct Config
        {
            const VulkanHelper::Image* image;
            ImageView::ViewType ViewType;

            uint32_t BaseLayer = 0;
            uint32_t LayerCount = UINT32_MAX;
        };

        [[nodiscard]] static Expected<ImageView, VHResult> New(const Config& config);

        ImageView(const ImageView& other) = delete;
        ImageView& operator=(const ImageView& other) = delete;

        ImageView(ImageView&& other) noexcept;
        ImageView& operator=(ImageView&& other) noexcept;

        ~ImageView();

        [[nodiscard]] Format GetFormat() const;
        [[nodiscard]] Image::Layout GetLayout() const;
        [[nodiscard]] MemoryProperties GetMemoryProperties() const;
        [[nodiscard]] Image::Aspect GetAspect() const;
        
        [[nodiscard]] uint32_t GetWidth() const;
        [[nodiscard]] uint32_t GetHeight() const;
        [[nodiscard]] uint32_t GetLayerCount() const;
        [[nodiscard]] uint32_t GetMipCount() const;

        class Impl;
    private:
        UniquePtr<Impl> m_Impl;

        ImageView(UniquePtr<Impl>&& impl);
    };
}