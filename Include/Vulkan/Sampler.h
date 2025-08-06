#pragma once

#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Expected.h"

#include "Vulkan/Device.h"

namespace VulkanHelper
{
    /**
     * @class Sampler
     * @brief RAII wrapper for Vulkan sampler object
     */
    class Sampler
    {
    public:

        /**
         * @enum AddressMode
         * @brief Defines how texture coordinates outside [0,1] range are handled
         */
        enum class AddressMode
        {
            REPEAT = 0,                ///< Repeat the texture (wrapping)
            MIRRORED_REPEAT = 1,       ///< Repeat with mirroring at boundaries
            CLAMP_TO_EDGE = 2,         ///< Clamp coordinates to edge texel
            CLAMP_TO_BORDER = 3,       ///< Clamp to border color
            MIRROR_CLAMP_TO_EDGE = 4,  ///< Mirror once then clamp to edge
            UNDEFINED = 0x7FFFFFFF     ///< Undefined/invalid mode
        };

        /**
         * @enum Filter
         * @brief Defines the filtering method used for texture sampling
         */
        enum class Filter
        {
            NEAREST = 0,           ///< Nearest neighbor filtering (no interpolation)
            LINEAR = 1,            ///< Linear interpolation filtering
            UNDEFINED = 0x7FFFFFFF ///< Undefined/invalid filter mode
        };

        /**
         * @enum MipmapMode
         * @brief Defines how mipmaps are sampled
         */
        enum class MipmapMode
        {
            NEAREST = 0,           ///< Use nearest mipmap level
            LINEAR = 1,            ///< Linear interpolation between mipmap levels
            UNDEFINED = 0x7FFFFFFF ///< Undefined/invalid mipmap mode
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a sampler
         */
        struct Config
        {
            /**
             * @brief Device to create the sampler on
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;

            /**
             * @brief Addressing mode for texture coordinates
             * @note Determines how coordinates outside [0,1] range are handled
             */
            Sampler::AddressMode AddressMode = Sampler::AddressMode::REPEAT;

            /**
             * @brief Minification filter mode
             * @note Used when texture appears smaller than its original size
             */
            Sampler::Filter MinFilter = Sampler::Filter::LINEAR;

            /**
             * @brief Magnification filter mode
             * @note Used when texture appears larger than its original size
             */
            Sampler::Filter MagFilter = Sampler::Filter::LINEAR;

            /**
             * @brief Mipmap sampling mode
             * @note Determines how different mipmap levels are sampled
             */
            Sampler::MipmapMode MipmapMode = Sampler::MipmapMode::LINEAR;
        };

        /**
         * @brief Creates a new sampler with the specified configuration
         * 
         * @param config Configuration parameters for the sampler
         * @return Expected containing the created sampler or an error code
         * @note The device must be valid and all filter/addressing modes must be supported
         */
        [[nodiscard]] static Expected<Sampler, VHResult> New(const Config& config);

        /**
         * @brief Delete copy constructor
         */
        Sampler(const Sampler& other) = delete;

        /**
         * @brief Delete copy assignment operator
         */
        Sampler& operator=(const Sampler& other) = delete;

        /**
         * @brief Move constructor
         */
        Sampler(Sampler&& other) noexcept;

        /**
         * @brief Move assignment operator
         */
        Sampler& operator=(Sampler&& other) noexcept;

        /**
         * @brief Destructor
         */
        ~Sampler();

        class Impl;
    private:
        friend class Impl;
        UniquePtr<Impl> m_Impl;

        Sampler(UniquePtr<Impl>&& impl);
    };
}