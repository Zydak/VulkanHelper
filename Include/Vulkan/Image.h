#pragma once
#include "Core/Expected.h"
#include "Core/Macros.h"
#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Enums.h"

namespace VulkanHelper
{
    class Device;
    class CommandBuffer;

    /**
     * @class Image
     * @brief RAII wrapper for Vulkan image objects
     * 
     * Provides a safe abstraction over VkImage with automatic memory allocation
     * and state management.
     */
    class Image
    {
    public:
        /**
         * @enum Usage
         * @brief Specifies how the image will be used in the graphics pipeline
         * 
         * These flags must be set at image creation time and can be combined using
         * bitwise operations.
         */
        enum class Usage
        {
            TRANSFER_SRC_BIT = 0x00000001,              ///< Image can be used as source for transfer operations
            TRANSFER_DST_BIT = 0x00000002,              ///< Image can be used as destination for transfer operations
            SAMPLED_BIT = 0x00000004,                   ///< Image can be sampled in shader programs
            STORAGE_BIT = 0x00000008,                   ///< Image can be used as storage image
            COLOR_ATTACHMENT_BIT = 0x00000010,          ///< Image can be used as color attachment
            DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,  ///< Image can be used as depth/stencil attachment
            TRANSIENT_ATTACHMENT_BIT = 0x00000040,      ///< Image will be used for temporary attachments
            INPUT_ATTACHMENT_BIT = 0x00000080,          ///< Image can be used as input attachment
            UNDEFINED = 0x7FFFFFFF                       ///< Undefined usage
        };

        /**
         * @brief Defines the aspect(s) of the image to be accessed
         *
         * Specifies which aspects of the image (color, depth, stencil, etc.)
         * will be accessed in various operations.
         */
        enum class Aspect
        {
            NONE = 0,                    ///< No aspect specified
            COLOR_BIT = 0x00000001,      ///< Color aspect of the image
            DEPTH_BIT = 0x00000002,      ///< Depth aspect of the image
            STENCIL_BIT = 0x00000004,    ///< Stencil aspect of the image
            METADATA_BIT = 0x00000008,   ///< Metadata aspect of the image
            PLANE_0_BIT = 0x00000010,    ///< First plane of a multi-planar image
            PLANE_1_BIT = 0x00000020,    ///< Second plane of a multi-planar image
            PLANE_2_BIT = 0x00000040,    ///< Third plane of a multi-planar image
            UNDEFINED = 0x7FFFFFFF       ///< Undefined aspect
        };

        /**
         * @brief Specifies the tiling arrangement of data in the image
         *
         * Determines how the image data is arranged in memory, affecting
         * performance and compatibility with certain operations.
         */
        enum class Tiling
        {
            OPTIMAL = 0,                 ///< Implementation-defined tiling, optimal for device access
            LINEAR = 1,                  ///< Row-major layout, suitable for CPU access
            UNDEFINED = 0x7FFFFFFF       ///< Undefined tiling mode
        };

        /**
         * @brief Specifies the layout of the image data in memory
         *
         * Defines the current layout of the image data, which affects how the image
         * can be accessed and what operations can be performed on it.
         */
        enum class Layout
        {
            UNDEFINED = 0,
            GENERAL = 1,
            COLOR_ATTACHMENT_OPTIMAL = 2,
            DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
            DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
            SHADER_READ_ONLY_OPTIMAL = 5,
            TRANSFER_SRC_OPTIMAL = 6,
            TRANSFER_DST_OPTIMAL = 7,
            PREINITIALIZED = 8,
            DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
            DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
            DEPTH_ATTACHMENT_OPTIMAL = 1000241000,
            DEPTH_READ_ONLY_OPTIMAL = 1000241001,
            STENCIL_ATTACHMENT_OPTIMAL = 1000241002,
            STENCIL_READ_ONLY_OPTIMAL = 1000241003,
            READ_ONLY_OPTIMAL = 1000314000,
            ATTACHMENT_OPTIMAL = 1000314001,
            PRESENT_SRC_KHR = 1000001002,
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a new Image instance
         *
         * Contains all necessary parameters for image creation and initialization.
         */
        struct Config
        {
            /**
             * @brief The Vulkan device to create the image on
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief Height of the image in pixels
             * @note Must be greater than 0 for 2D/3D images, or 1 for 1D images
             */
            uint32_t Height = 0;

            /**
             * @brief Width of the image in pixels
             * @note Must be greater than 0
             */
            uint32_t Width = 0;

            /**
             * @brief Number of mipmap levels
             * @note Must be at least 1, or log2(max(width,height)) + 1 for a complete mip chain
             */
            uint32_t MipCount = 1;

            /**
             * @brief Number of array layers
             * @note Must be at least 1, or 6 for cubemaps
             */
            uint32_t LayerCount = 1;

            /**
             * @brief Format of the image data
             * @note Must not be Format::UNDEFINED
             */
            VulkanHelper::Format Format = Format::UNDEFINED;

            /**
             * @brief Usage flags defining how the image will be used
             * @note Must not be Usage::UNDEFINED
             */
            VulkanHelper::Image::Usage Usage = Usage::UNDEFINED;

            /**
             * @brief Tiling mode for the image data
             * @note OPTIMAL is preferred for GPU-only access, LINEAR for CPU access
             */
            VulkanHelper::Image::Tiling Tiling = Tiling::OPTIMAL;

            /**
             * @brief Aspect flags for the image
             * @note Must match the aspect(s) implied by the Format
             */
            VulkanHelper::Image::Aspect Aspect = Aspect::COLOR_BIT;

            /**
             * @brief Initial layout of the image
             * @note Must be valid Image::Layout
             */
            VulkanHelper::Image::Layout InitialLayout = Layout::UNDEFINED;

            /**
             * @brief Sample count for the image
             * @note Defaults to COUNT_1_BIT, can be set to other values for multisampling
             */
            VulkanHelper::SampleCount SampleCount = SampleCount::COUNT_1_BIT;

            /**
             * @brief Whether to use persistent staging
             * @note Improves performance for frequent CPU writes to GPU-only images, but doubles the memory size cost
             */
            bool UsePersistentStagingBuffer = false;

            /**
             * @brief Flag dictating whether you can map image memory
             */
            bool AllowMapping = false;
        };

        /**
         * @brief Creates a new Image instance with the specified configuration
         *
         * This static factory function attempts to create an image with the given dimensions,
         * format, usage flags, and memory properties. Memory is automatically allocated based
         * on the specified configuration. The image is created in an undefined layout and
         * must be transitioned to a valid layout before use.
         *
         * @param config Configuration struct specifying image parameters
         * @return Expected<Image, VHResult> An expected containing the created Image on success,
         *         or a VHResult error code on failure
         */
        [[nodiscard]] static Expected<Image, VHResult> New(const Config& config);

        /**
         * @brief Delete copy constructor
         */
        Image(const Image& other) = delete;

        /**
         * @brief Delete copy assignment operator
         */
        Image& operator=(const Image& other) = delete;
        
        /**
         * @brief Move constructor
         */
        Image(Image&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        Image& operator=(Image&& other) noexcept;

        /**
         * @brief Destructor
         */
        ~Image();

        /**
         * @brief Records a layout transition command to the specified command buffer
         *
         * @param newLayout The new layout to transition to
         * @param commandBuffer The command buffer to record the commands into
         * @param baseLayer The first array layer to transition
         * @param layerCount The number of consecutive layers to transition
         */
        void TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer = 0, uint32_t layerCount = 1);

        /**
         * @brief Upload data to the image.
         *
         * @param data Pointer to the data to upload.
         * @param size Size of the data in bytes.
         * @param offset Offset in the image to start writing.
         * @param cmd Command buffer for GPU operations (required for non-mappable images).
         * @return VHResult::OK on success, or an error code on failure.
         */
        VHResult UploadData(const void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd = nullptr);

        /**
         * @brief Download data from the image.
         *
         * @param data Pointer to the destination buffer.
         * @param size Size of the data to read in bytes.
         * @param offset Offset in the image to start reading.
         * @param cmd Command buffer for GPU operations (required for non-mappable images).
         * @return VHResult::OK on success, or an error code on failure.
         */
        VHResult DownloadData(void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd = nullptr) const;

        /**
         * @brief Map image memory for CPU access
         * @return Expected containing pointer to mapped memory or an error code
         * @note Image must be created with AllowMapping=true
         */
        [[nodiscard]] Expected<void*, VHResult> Map();

        /**
         * @brief Unmap previously mapped image memory
         * @note Must be called after Map() when done accessing the memory
         */
        void Unmap();
        
        /**
         * @brief Gets the format of the image
         * @return The image format
         */
        [[nodiscard]] Format GetFormat() const;

        /**
         * @brief Gets the current layout of the image
         * @return The current image layout
         */
        [[nodiscard]] Layout GetLayout() const;

        /**
         * @brief Gets the aspect flags of the image
         * @return The image aspect flags
         */
        [[nodiscard]] Aspect GetAspect() const;
        
        /**
         * @brief Gets the width of the image in pixels
         * @return The image width
         */
        [[nodiscard]] uint32_t GetWidth() const;

        /**
         * @brief Gets the height of the image in pixels
         * @return The image height
         */
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * @brief Gets the number of array layers in the image
         * @return The number of layers
         */
        [[nodiscard]] uint32_t GetLayerCount() const;

        /**
         * @brief Gets the number of mipmap levels in the image
         * @return The number of mip levels
         */
        [[nodiscard]] uint32_t GetMipCount() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Image(UniquePtr<Impl> && impl);

        friend class Swapchain; // Allow Swapchain to construct Image instances
    };

    DEFINE_BITWISE_OPERATORS(Image::Usage)
    DEFINE_BITWISE_OPERATORS(Image::Aspect)
}