#pragma once

#include "Core/Expected.h"
#include "Core/Macros.h"
#include "Core/SharedPtr.h"
#include "Core/Error.h"
#include "Core/Enums.h"

#include "Vulkan/Device.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Image.h"

namespace VulkanHelper
{
    /**
     * @class Buffer
     * @brief RAII wrapper for Vulkan buffer objects
     */
    class Buffer
    {
    public:
        /**
         * @enum Usage
         * @brief Specifies how the buffer can be used in the pipeline
         */
        enum class Usage
        {
            NONE = 0,
            TRANSFER_SRC_BIT            = 0x00000001,  ///< Source for transfer operations
            TRANSFER_DST_BIT            = 0x00000002,  ///< Destination for transfer operations
            UNIFORM_TEXEL_BUFFER_BIT    = 0x00000004, ///< Uniform texel buffer
            STORAGE_TEXEL_BUFFER_BIT    = 0x00000008, ///< Storage texel buffer
            UNIFORM_BUFFER_BIT          = 0x00000010,  ///< Uniform buffer for shader input
            STORAGE_BUFFER_BIT          = 0x00000020,  ///< Storage buffer for shader read/write
            INDEX_BUFFER_BIT            = 0x00000040,  ///< Index buffer for drawing
            VERTEX_BUFFER_BIT           = 0x00000080,  ///< Vertex buffer for drawing
            INDIRECT_BUFFER_BIT         = 0x00000100,  ///< Buffer for indirect draw commands

            SHADER_DEVICE_ADDRESS_BIT = 0x00020000ULL, ///< You can get a device address for this buffer

            ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT = 0x00080000, ///< Read-only input for acceleration structure builds
            ACCELERATION_STRUCTURE_STORAGE_BIT = 0x00100000, ///< Storage for acceleration structures
            SHADER_BINDING_TABLE_BIT = 0x00000400, ///< Shader binding table for ray tracing
            UNDEFINED = 0x7FFFFFFF           ///< Invalid usage
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a buffer
         */
        struct Config
        {
            /**
             * @brief Device to create the buffer on
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Size of the buffer in bytes
             * @note Must be greater than 0
             */
            uint64_t Size = 0;

            /**
             * @brief Usage flags for the buffer
             * @note Must not be Usage::UNDEFINED
             */
            VulkanHelper::Buffer::Usage Usage = Usage::UNDEFINED;

            /**
             * @brief Whether buffer memory can be mapped
             * @note Required for direct CPU access
             */
            bool CpuMapable = false;

            /**
             * @brief Minimum alignment for the buffer memory
             */
            uint32_t MinAlignment = 1;

            /**
             * @brief Optional debug name
             */
            const char* DebugName = "";

            /**
             * @brief Number of frames to wait before deleting the buffer
             */
            uint32_t DeleteDelayInFrames = 1;
        };

        /**
         * @brief Creates a new buffer
         * @param config Configuration parameters for the buffer
         * @return Expected containing the created buffer or an error code
         */
        [[nodiscard]] static Expected<Buffer, VHResult> New(const Config& config);

        Buffer();

        Buffer(const Buffer& other);
        Buffer& operator=(const Buffer& other);
        
        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;

        ~Buffer();

        [[nodiscard]] bool operator==(std::nullptr_t) const
        {
            return m_Impl == nullptr;
        }

        [[nodiscard]] bool operator!=(std::nullptr_t) const
        {
            return m_Impl != nullptr;
        }

        /**
         * @brief Upload data to the buffer
         * @param data Pointer to source data
         * @param size Size in bytes to upload
         * @param offset Destination offset in bytes
         * @return VHResult::OK on success
         * 
         * @note buffer must have been created with CpuMapable=true to use this function
         */
        [[nodiscard]] VHResult UploadData(const void* data, uint64_t size, uint64_t offset = 0);

        /**
         * @brief Download data from the buffer
         * @param data Pointer to destination buffer
         * @param size Size in bytes to download
         * @param offset Source offset in bytes
         * @return VHResult::OK on success
         * 
         * @note buffer must have been created with CpuMapable=true to use this function
         */
        [[nodiscard]] VHResult DownloadData(void* data, uint64_t size, uint64_t offset = 0) const;

        /**
         * @brief Map buffer memory for CPU access
         * @return Expected containing pointer to mapped memory or an error code
         * @note Buffer must be created with CpuMapable=true
         */
        [[nodiscard]] Expected<void*, VHResult> Map();

        /**
         * @brief Unmap previously mapped buffer memory
         * @note Must be called after Map() when done accessing the memory
         */
        void Unmap();

        /**
         * @brief Copy data from another buffer using GPU commands
         * @param cmd Command buffer to record the copy into
         * @param source Source buffer
         * @param srcOffset Source offset in bytes
         * @param dstOffset Destination offset in bytes
         * @param size Size to copy in bytes (UINT64_MAX for entire buffer)
         * @return VHResult::OK on success
         */
        [[nodiscard]] VHResult CopyFromBuffer(
            CommandBuffer& cmd,
            const Buffer& source, 
            uint64_t srcOffset = 0,
            uint64_t dstOffset = 0, 
            uint64_t size = UINT64_MAX
        );

        /**
         * @brief Copy data to another buffer using GPU commands
         * @param cmd Command buffer to record the copy into
         * @param destination Destination buffer
         * @param srcOffset Source offset in bytes
         * @param dstOffset Destination offset in bytes
         * @param size Size to copy in bytes (UINT64_MAX for entire buffer)
         * @return VHResult::OK on success
         */
        [[nodiscard]] VHResult CopyToBuffer(
            CommandBuffer& cmd,
            const Buffer& destination, 
            uint64_t srcOffset = 0,
            uint64_t dstOffset = 0, 
            uint64_t size = UINT64_MAX
        );

        /**
         * @brief Copy buffer data to an image
         * @param cmd Command buffer to record the copy into
         * @param dst Destination image
         * @param bufferOffset Offset in the buffer to start copying from
         * @param imageOffsetX X offset in the image to start copying to
         * @param imageOffsetY Y offset in the image to start copying to
         * @param imageExtentX Width of the image region to copy to (or UINT32_MAX for full width)
         * @param imageExtentY Height of the image region to copy to (or UINT32_MAX for full height)
         * @param imageBaseLayer First layer in the image to copy to
         * @param layerCount Number of layers to copy to
         * @return VHResult::OK on success
         */
        [[nodiscard]] VHResult CopyToImage(
            CommandBuffer& cmd,
            Image& dst,
            uint32_t bufferOffset = 0,
            uint32_t imageOffsetX = 0,
            uint32_t imageOffsetY = 0,
            uint32_t imageExtentX = UINT32_MAX,
            uint32_t imageExtentY = UINT32_MAX,
            uint32_t imageBaseLayer = 0,
            uint32_t layerCount = 1
        );


        /**
         * @brief Copy image data to the buffer
         * @param cmd Command buffer to record the copy into
         * @param source Source image
         * @param bufferOffset Offset in the buffer to start copying to
         * @param imageOffsetX X offset in the image to start copying from
         * @param imageOffsetY Y offset in the image to start copying from
         * @param imageExtentX Width of the image region to copy from (or UINT32_MAX for full width)
         * @param imageExtentY Height of the image region to copy from (or UINT32_MAX for full height)
         * @param imageBaseLayer First layer in the image to copy from
         * @param layerCount Number of layers to copy from
         * @return VHResult::OK on success
         */
        [[nodiscard]] VHResult CopyFromImage(
            CommandBuffer& cmd,
            Image& source,
            uint32_t bufferOffset = 0,
            uint32_t imageOffsetX = 0,
            uint32_t imageOffsetY = 0,
            uint32_t imageExtentX = UINT32_MAX,
            uint32_t imageExtentY = UINT32_MAX,
            uint32_t imageBaseLayer = 0,
            uint32_t layerCount = 1
        );

        /**
         * @brief Get the buffer size
         * @return Size in bytes
         */
        [[nodiscard]] uint64_t GetSize() const;

        /**
         * @brief Get the buffer usage flags
         * @return Usage flags
         */
        [[nodiscard]] Usage GetUsage() const;

        /**
         * @brief Check if buffer memory is mapped
         * @return true if mapped
         */
        [[nodiscard]] bool IsMapped() const;
        
        void Barrier(CommandBuffer& cmd, AccessFlags srcAccess, AccessFlags dstAccess, PipelineStages srcStage, PipelineStages dstStage);

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        Buffer(const SharedPtr<Impl>& impl);
    };

    DEFINE_BITWISE_OPERATORS(Buffer::Usage)
}