#pragma once

#include "BLAS.h"
#include "Core/Error.h"
#include "Core/Expected.h"

namespace VulkanHelper
{
    /**
     * @class BLASBuilder
     * @brief Helper class to build multiple BLAS (Bottom Level Acceleration Structure) objects at once
     */
    class BLASBuilder
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a BLASBuilder
         */
        struct Config
        {
            /**
             * @brief Device to create the BLAS on
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};
        };

        /**
         * @brief Creates a new BLASBuilder
         * @param config Configuration parameters for the BLASBuilder
         * @return Expected containing the created BLASBuilder or an error code
         */
        [[nodiscard]] static Expected<BLASBuilder, VHResult> New(const Config& config);

        /**
         * @brief Builds multiple BLAS objects using the provided configurations and command buffer
         * @param blasConfigs Array of BLAS configuration parameters
         * @param count Number of BLAS configurations in the array
         * @param commandBuffer Command buffer to use for the build process
         * @return Expected containing a vector of created BLAS objects or an error code
         */
        [[nodiscard]] Expected<Vector<BLAS>, VHResult> Build(VulkanHelper::BLAS::Config* blasConfigs, uint32_t count, CommandBuffer& commandBuffer);

        /**
         * @brief Compacts the provided BLAS objects to save memory
         * @param blasList Vector of BLAS objects to compact
         * @param commandBuffer Command buffer to use for the compaction
         * @return VHResult indicating success or failure of the operation
         */
        [[nodiscard]] VHResult Compact(Vector<BLAS>& blasList, CommandBuffer& commandBuffer);

        BLASBuilder();

        BLASBuilder(const BLASBuilder& other);
        BLASBuilder& operator=(const BLASBuilder& other);

        BLASBuilder(BLASBuilder&& other) noexcept;
        BLASBuilder& operator=(BLASBuilder&& other) noexcept;

        ~BLASBuilder();

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        BLASBuilder(const SharedPtr<Impl>& impl);
    };
}