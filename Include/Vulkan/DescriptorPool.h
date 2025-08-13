#pragma once

#include "Core/Error.h"
#include "Core/SharedPtr.h"
#include "Core/Expected.h"
#include "Core/Vector.h"
#include "Core/Enums.h"

#include "Vulkan/DescriptorSet.h"
#include "Vulkan/Device.h"

namespace VulkanHelper
{
    /**
     * @class DescriptorPool
     * @brief RAII wrapper for Vulkan descriptor pool object
     */
    class DescriptorPool
    {
    public:
        /**
         * @struct PoolSize
         * @brief Specifies the number of descriptors of a particular type to allocate
         */
        struct PoolSize
        {
            /**
             * @brief Type of descriptor to allocate
             * @note Must be a valid descriptor type (e.g., uniform buffer, sampler, etc.)
             */
            DescriptorType Type = DescriptorType::UNDEFINED;

            /**
             * @brief Number of descriptors of this type to allocate in the pool
             * @note Must be greater than 0
             */
            uint32_t DescriptorCount = 0;
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a descriptor pool
         */
        struct Config
        {
            /**
             * @brief Device to create the descriptor pool on
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Maximum number of descriptor sets that can be allocated from this pool
             * @note This is the total limit, not per descriptor type
             */
            uint32_t MaxSets = 1;

            /**
             * @brief Array of pool sizes specifying how many descriptors of each type to allocate
             * @note Must contain at least one entry with DescriptorCount > 0
             */
            const PoolSize* PoolSizes = nullptr;

            /**
             * @brief Number of entries in the PoolSizes array
             * @note Must be greater than 0
             */
            uint32_t PoolSizeCount = 0;
        };

        /**
         * @brief Creates a new descriptor pool with the specified configuration
         * 
         * @param config Configuration parameters for the descriptor pool
         * @return Expected containing the created descriptor pool or an error code
         * @note The device must be valid and pool sizes must be properly configured
         */
        [[nodiscard]] static Expected<DescriptorPool, VHResult> New(const Config& config);

        DescriptorPool();

        DescriptorPool(const DescriptorPool& other);
        DescriptorPool& operator=(const DescriptorPool& other);

        DescriptorPool(DescriptorPool&& other) noexcept;
        DescriptorPool& operator=(DescriptorPool&& other) noexcept;

        ~DescriptorPool();

        /**
         * @brief Allocates a descriptor set from this pool
         * 
         * @param config Configuration describing the layout and bindings for the descriptor set
         * @return Expected containing the allocated descriptor set or an error code
         * @note The pool must have sufficient free descriptors of the required types
         * @note The allocated descriptor set will be automatically freed when the pool is destroyed
         */
        [[nodiscard]] Expected<DescriptorSet, VHResult> AllocateDescriptorSet(const DescriptorSet::Config& config);

        /**
         * @brief Resets the descriptor pool, freeing all allocated descriptor sets
         * 
         * @note All descriptor sets allocated from this pool become invalid after reset
         * @note This is more efficient than freeing descriptor sets individually
         */
        void Reset();

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        DescriptorPool(const SharedPtr<Impl>& impl);
    };
}