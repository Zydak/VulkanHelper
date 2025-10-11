#pragma once

#include "Core/Expected.h"
#include "Core/Macros.h"
#include "Core/SharedPtr.h"
#include "Core/Error.h"
#include "Core/Enums.h"

#include "Vulkan/Device.h"
#include "Vulkan/Buffer.h"

namespace VulkanHelper
{
    /**
     * @class BLAS
     * @brief RAII wrapper for Vulkan BLAS (Bottom Level Acceleration Structure) objects
     * 
     * Manages geometric data for ray tracing acceleration structures.
     */
    class BLAS
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a BLAS
         */
        struct Config
        {
            /**
             * @brief Device to create the BLAS on
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Vertex buffers containing geometry data
             * @note Must not be empty and all buffers must be valid
             */
            VulkanHelper::Vector<VulkanHelper::Buffer> VertexBuffers;

            /**
             * @brief Size of each vertex in bytes
             * @note Must match the actual size of vertices in VertexBuffers
             */
            uint32_t VertexSize = 0;

            /**
             * @brief Index buffers for indexed geometry (optional)
             * @note If used, the count must match VertexBuffers count, fill with nullptrs if not used
             */
            VulkanHelper::Vector<VulkanHelper::Buffer> IndexBuffers;

            /**
             * @brief Whether to compact the acceleration structure after building
             * @note Compaction reduces memory usage but requires additional GPU work during creation.
             *       The acceleration structure will be built twice: once to get compaction info, then again in the compacted form.
             */
            bool EnableCompaction = false;
        };

        BLAS();

        BLAS(const BLAS& other);
        BLAS& operator=(const BLAS& other);

        BLAS(BLAS&& other) noexcept;
        BLAS& operator=(BLAS&& other) noexcept;

        ~BLAS();

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        BLAS(const SharedPtr<Impl>& impl);
    };
}