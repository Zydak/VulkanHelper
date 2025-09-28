#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/SharedPtr.h"

#include "Vulkan/BLAS.h"
#include <cstdint>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"

namespace VulkanHelper
{
    /**
     * @class TLAS
     * @brief RAII wrapper for Vulkan TLAS (Top Level Acceleration Structure) objects
     * 
     * Manages instances of BLAS objects for ray tracing operations.
     */
    class TLAS
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a TLAS
         */
        struct Config
        {
            /**
             * @brief Device to create the TLAS on
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief List of BLAS objects to instance in this TLAS
             * @note Must not be empty and all BLAS objects must be valid
             */
            Vector<VulkanHelper::BLAS> BlasList;

            /**
             * @brief Custom indices for each BLAS instance
             * @note Array size must match BlasList size.
             */
            Vector<uint32_t> InstanceCustomIndices;

            /**
             * @brief Transform matrices for each BLAS instance
             * @note Array size must match BlasList size
             */
            const glm::mat4* Transforms;

            /**
             * @brief Command buffer for TLAS construction
             * @note Must not be nullptr and must be in recording state
             */
            VulkanHelper::CommandBuffer* CommandBuffer;
        };

        /**
         * @brief Creates a new TLAS
         * @param config Configuration parameters for the TLAS
         * @return Expected containing the created TLAS or an error code
         */
        [[nodiscard]] static Expected<TLAS, VHResult> New(const Config& config);

        TLAS();

        TLAS(const TLAS& other);
        TLAS& operator=(const TLAS& other);

        TLAS(TLAS&& other) noexcept;
        TLAS& operator=(TLAS&& other) noexcept;

        ~TLAS();

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        TLAS(const SharedPtr<Impl>& impl);
    };
}