#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/SharedPtr.h"

#include "Vulkan/Image.h"

namespace VulkanHelper
{
    /**
     * @class ImageView
     * @brief RAII wrapper for Vulkan image view objects
     * 
     * Provides a way to interpret image data with specific format and dimensions.
     */
    class ImageView
    {
    public:
        /**
         * @enum ViewType
         * @brief Specifies how the image data should be interpreted
         * 
         * Must match the dimensionality of the source image.
         */
        enum class ViewType
        {
            VIEW_1D = 0,            ///< One-dimensional image view
            VIEW_2D = 1,            ///< Two-dimensional image view
            VIEW_3D = 2,            ///< Three-dimensional image view
            VIEW_CUBE = 3,          ///< Cube map image view
            VIEW_1D_ARRAY = 4,      ///< Array of one-dimensional images
            VIEW_2D_ARRAY = 5,      ///< Array of two-dimensional images
            VIEW_CUBE_ARRAY = 6,    ///< Array of cube maps
            VIEW_UNDEFINED = 0x7FFFFFFF  ///< Undefined view type
        };
        
        /**
         * @brief Configuration structure for creating a new ImageView
         *
         * Contains all necessary parameters to create and initialize a new ImageView instance.
         */
        /**
         * @struct Config
         * @brief Configuration parameters for creating a new ImageView instance
         */
        struct Config
        {
            /**
             * @brief Source image to create the view from
             * @note Must be a valid image
             */
            VulkanHelper::Image image;

            /**
             * @brief Type of view to create
             * @note Must not be ViewType::UNDEFINED and must be compatible with image dimensions
             */
            ImageView::ViewType ViewType;

            /**
             * @brief First array layer accessible to the view
             * @note Must be less than the image's layer count
             */
            uint32_t BaseLayer = 0;

            /**
             * @brief Number of consecutive array layers to include in the view
             * @note Must not exceed (image layer count - base layer)
             */
            uint32_t LayerCount = UINT32_MAX;
        };

        /**
         * @brief Creates a new ImageView instance
         * @param config Configuration parameters for the image view
         * @return Expected containing the created ImageView or an error code
         * @note The source image must remain valid for the lifetime of the view
         */
        [[nodiscard]] static Expected<ImageView, VHResult> New(const Config& config);

        ImageView();

        ImageView(const ImageView& other);
        ImageView& operator=(const ImageView& other);

        ImageView(ImageView&& other) noexcept;
        ImageView& operator=(ImageView&& other) noexcept;

        ~ImageView();

        /**
         * @brief Gets the underlying image of the view
         * @return The image associated with this view
         */
        [[nodiscard]] Image GetImage() const;

        /**
         * @brief Gets the format of the view's image
         * @return The image format
         */
        [[nodiscard]] Format GetFormat() const;

        /**
         * @brief Gets the current layout of the view's image
         * @return The current image layout
         */
        [[nodiscard]] Image::Layout GetLayout() const;

        /**
         * @brief Gets the aspect flags of the view's image
         * @return The image aspect flags
         */
        [[nodiscard]] Image::Aspect GetAspect() const;
        
        /**
         * @brief Gets the width of the image view in pixels
         * @return The image view width
         */
        [[nodiscard]] uint32_t GetWidth() const;

        /**
         * @brief Gets the height of the image view in pixels
         * @return The image view height
         */
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * @brief Gets the number of array layers in the image view
         * @return The number of layers
         */
        [[nodiscard]] uint32_t GetLayerCount() const;

        /**
         * @brief Gets the number of mipmap levels in the image view
         * @return The number of mip levels
         */
        [[nodiscard]] uint32_t GetMipCount() const;

        class Impl;
    private:
        SharedPtr<Impl> m_Impl;

        ImageView(const SharedPtr<Impl>& impl);
    };
}