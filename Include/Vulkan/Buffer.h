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
     * 
     * Manages memory-backed linear storage for vertex, index, uniform and other data.
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
             * @brief Whether to use persistent staging
             * @note Improves performance for frequent CPU writes to GPU-only buffers, but doubles the memory size cost
             */
            bool UsePersistentStagingBuffer = false;

            uint32_t MinAlignment = 1; ///< Minimum alignment for buffer allocation

            /**
             * @brief Optional debug name
             */
            const char* DebugName = "";
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

        /**
         * @brief Upload data to the buffer
         * @param data Pointer to source data
         * @param size Size in bytes to upload
         * @param offset Destination offset in bytes
         * @param cmd Command buffer for GPU transfers (required for non-mappable buffers)
         * @return VHResult::OK on success
         * 
         * @note If you pass a command buffer in, you must ensure it is executed before the end of the frame.
         */
        [[nodiscard]] VHResult UploadData(const void* data, uint64_t size, uint64_t offset = 0, CommandBuffer* cmd = nullptr);

        /**
         * @brief Download data from the buffer
         * @param data Pointer to destination buffer
         * @param size Size in bytes to download
         * @param offset Source offset in bytes
         * @param cmd Command buffer for GPU transfers (required for non-mappable buffers)
         * @return VHResult::OK on success
         */
        [[nodiscard]] VHResult DownloadData(void* data, uint64_t size, uint64_t offset = 0, CommandBuffer* cmd = nullptr) const;

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
        [[nodiscard]] VHResult CopyFrom(CommandBuffer& cmd, const Buffer& source, 
                         uint64_t srcOffset = 0, uint64_t dstOffset = 0, 
                         uint64_t size = UINT64_MAX);

        /**
         * @brief Copy buffer data to an image
         * @param cmd Command buffer to record the copy into
         * @param dst Destination image
         * @param bufferOffset Source offset in bytes
         * @param bufferRowLength Row length in pixels (0 for tightly packed)
         * @param bufferImageHeight Image height in pixels (0 for tightly packed)
         * @return VHResult::OK on success
         */
        [[nodiscard]] VHResult CopyToImage(CommandBuffer& cmd, const Image& dst, 
                            uint32_t bufferOffset = 0, uint32_t bufferRowLength = 0, 
                            uint32_t bufferImageHeight = 0);


        /**
         * @brief Copy image data to the buffer
         * @param cmd Command buffer to record the copy into
         * @param src Source image
         * 
         * @return VHResult::OK on success
         *
         * @note Buffer has to be exactly the same size as the image, copying with offsets is not implemented yet
         */
        [[nodiscard]] VHResult CopyFromImage(CommandBuffer& cmd, const Image& src);

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