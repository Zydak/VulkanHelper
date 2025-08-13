#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/SharedPtr.h"
#include "Core/Enums.h"

#include "CommandBuffer.h"

namespace VulkanHelper
{
    /**
     * @class PushConstant
     * @brief RAII wrapper for Vulkan push constant data
     * 
     * Manages small blocks of data that can be rapidly updated and pushed to shaders.
     * Push constants are automatically pushed to the GPU when a pipeline is bound using Pipeline::Bind().
     * They provide a fast way to update shader uniforms without descriptor sets.
     * 
     * @note Push constants have size limitations (typically 128-256 bytes) depending on the device.
     * @note Push constants are automatically pushed when Pipeline::Bind() is called.
     */
    class PushConstant
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a push constant
         */
        struct Config
        {
            /**
             * @brief Shader stages where this push constant will be used
             * @note Must not be ShaderStages::UNDEFINED
             */
            ShaderStages Stage = ShaderStages::UNDEFINED;

            /**
             * @brief Pointer to initial data
             * @note Can be nullptr, data can be set later with SetData()
             */
            void* Data = nullptr;

            /**
             * @brief Size of the push constant data in bytes
             * @note Must be greater than 0 and within device push constant size limits
             */
            uint32_t Size = 0;
        };

        /**
         * @brief Creates a new push constant
         * @param config Configuration parameters for the push constant
         * @return Expected containing the created push constant or an error code
         */
        [[nodiscard]] static Expected<PushConstant, VHResult> New(const Config& config);

        PushConstant();

        PushConstant(const PushConstant& other);
        PushConstant& operator=(const PushConstant& other);

        PushConstant(PushConstant&& other) noexcept;
        PushConstant& operator=(PushConstant&& other) noexcept;

        ~PushConstant();

        /**
         * @brief Update data in the push constant
         * @param data Pointer to source data
         * @param size Size in bytes to update
         * @param offset Destination offset in bytes within the push constant
         * @return VHResult::OK on success
         * @note The updated data will be pushed to shaders on the next Pipeline::Bind() call
         */
        [[nodiscard]] VHResult SetData(const void* data, uint32_t size, uint32_t offset = 0);

        /**
         * @brief Get the shader stages this push constant is used in
         * @return The shader stage flags
         */
        [[nodiscard]] ShaderStages GetStage() const;
        
        /**
         * @brief Get a pointer to the push constant data
         * @return Const pointer to the internal data
         */
        [[nodiscard]] const void* GetData() const;
        
        /**
         * @brief Get the size of the push constant data
         * @return Size in bytes
         */
        [[nodiscard]] uint32_t GetSize() const;

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        PushConstant(const SharedPtr<Impl>& impl);
    };
}