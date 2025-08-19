#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/SharedPtr.h"
#include "Core/Enums.h"
#include "Vulkan/Device.h"

namespace VulkanHelper
{
    /**
     * @class Shader
     * @brief RAII wrapper for a Vulkan shader module. Handles compilation and lifecycle of shader code.
     */
    class Shader
    {
    public:
        struct Define
        {
            const char* name = "";
            const char* value = "";
        };

        /**
         * @brief Configuration for shader module creation
         */
        struct Config
        {
            /**
             * @brief The logical device that will own this shader module
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Path to the shader source file
             */
            const char* Filepath = "";

            /**
             * @brief The type of shader (vertex, fragment, compute, etc.)
             */
            ShaderStages Stage = ShaderStages::UNDEFINED;
        };

        /**
         * @brief Sets up the shader compilation environment with the given search path.
         * 
         * @param shaderSearchPath Directory to search for shader files
         * @param definesCount Optional count of defines in the defines argument
         * @param defines Optional array of preprocessor definitions to use during shader compilation
         * 
         * @note Must be called before creating any shader modules
         */
        static void InitializeSession(const char* shaderSearchPath, uint32_t definesCount = 0, const Define* defines = nullptr);

        /**
         * @brief Creates a new shader module from the specified shader file.
         * 
         * @param config Shader creation configuration
         * @return Expected<Shader, VHResult> The created shader module or error code
         * @note Device must be valid and shader file must exist
         */
        [[nodiscard]] static Expected<Shader, VHResult> New(const Config& config); 

        Shader();

        Shader(const Shader& other);
        Shader& operator=(const Shader& other);

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;
        
        ~Shader();

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        Shader(const SharedPtr<Impl>& impl);
    };
}