#pragma once

#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Expected.h"
#include "Core/Enums.h"

#include "Vulkan/ImageView.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Sampler.h"

namespace VulkanHelper
{
    /**
     * @class DescriptorSet
     * @brief RAII wrapper for Vulkan descriptor set objects
     * 
     * Manages descriptor sets which bind resources (buffers, images, samplers) to shaders.
     * Descriptor sets are typically allocated from a DescriptorPool and contain bindings
     * that describe how shaders access resources.
     */
    class DescriptorSet
    {
    public:
        /**
         * @struct BindingDescription
         * @brief Describes a single binding within a descriptor set
         */
        struct BindingDescription
        {
            /**
             * @brief Binding index in the shader
             * @note Must match the binding number specified in the shader
             */
            uint32_t Binding = 0;

            /**
             * @brief Number of descriptors in this binding (for arrays)
             * @note For non-array bindings, this should be 1
             */
            uint32_t DescriptorsCount = 1;

            /**
             * @brief Shader stages that can access this binding
             * @note Can be a combination of vertex, fragment, compute, etc.
             */
            ShaderStages StageFlags = ShaderStages::UNDEFINED;

            /**
             * @brief Type of descriptor (uniform buffer, sampler, etc.)
             * @note Must match the descriptor type declared in the shader
             */
            DescriptorType Type = DescriptorType::UNDEFINED;
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a descriptor set
         */
        struct Config
        {
            /**
             * @brief Array of binding descriptions
             * @note Must not be nullptr and must contain valid binding descriptions
             */
            const BindingDescription* Bindings = nullptr;

            /**
             * @brief Number of bindings in the array
             * @note Must be greater than 0
             */
            uint32_t BindingCount = 0;
        };

        /**
         * @brief Delete copy constructor
         */
        DescriptorSet(const DescriptorSet& other) = delete;

        /**
         * @brief Delete copy assignment operator
         */
        DescriptorSet& operator=(const DescriptorSet& other) = delete;

        /**
         * @brief Move constructor
         */
        DescriptorSet(DescriptorSet&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        DescriptorSet& operator=(DescriptorSet&& other) noexcept;

        /**
         * @brief Destructor
         */
        ~DescriptorSet();

        /**
         * @brief Adds a buffer to the descriptor set at the specified binding
         * 
         * @param binding The binding index in the descriptor set layout
         * @param arrayIndex The array index within the binding (0 for non-array bindings)
         * @param buffer The buffer to bind
         * @return VHResult::OK on success, or an error code on failure
         * @note The binding must be configured for buffer descriptors
         */
        [[nodiscard]] VHResult AddBuffer(uint32_t binding, uint32_t arrayIndex, const Buffer& buffer);

        /**
         * @brief Adds an image to the descriptor set at the specified binding
         * 
         * @param binding The binding index in the descriptor set layout
         * @param arrayIndex The array index within the binding (0 for non-array bindings)
         * @param imageView The image view to bind
         * @param layout The expected layout of the image when accessed by shaders
         * @return VHResult::OK on success, or an error code on failure
         * @note The binding must be configured for image descriptors
         */
        [[nodiscard]] VHResult AddImage(uint32_t binding, uint32_t arrayIndex, const ImageView& imageView, Image::Layout layout);

        /**
         * @brief Adds a sampler to the descriptor set at the specified binding
         * 
         * @param binding The binding index in the descriptor set layout
         * @param arrayIndex The array index within the binding (0 for non-array bindings)
         * @param sampler The sampler to bind
         * @return VHResult::OK on success, or an error code on failure
         * @note The binding must be configured for sampler descriptors
         */
        [[nodiscard]] VHResult AddSampler(uint32_t binding, uint32_t arrayIndex, const Sampler& sampler);
        
        // TODO For Later
        //[[nodiscard]] VHResult AddAccelerationStructure(uint32_t binding, uint32_t arrayIndex, const AccelerationStructure* accelerationStructure);

        /**
         * @brief Creates a new descriptor set with the specified configuration
         * 
         * @param config Configuration parameters for the descriptor set
         * @return Expected containing the created descriptor set or an error code
         * @note This is primarily used internally. Use DescriptorPool::AllocateDescriptorSet() instead
         * @note Descriptor sets should typically be allocated from a DescriptorPool
         */
        [[nodiscard]] static Expected<UniquePtr<DescriptorSet>, VHResult> New(const Config& config);

        class Impl;
    private:
        friend class Impl;
        UniquePtr<Impl> m_Impl;

        /**
         * @brief Private constructor for internal use
         * @param impl Implementation pointer
         */
        DescriptorSet(UniquePtr<Impl>&& impl);

        friend class DescriptorPool; // Allow DescriptorPool to create DescriptorSets, Don't use static New
    };
}