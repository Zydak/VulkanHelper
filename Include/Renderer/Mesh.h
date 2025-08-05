#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Enums.h"

#include "Vulkan/Device.h"
#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    /**
     * @class Mesh
     * @brief RAII wrapper for Vulkan mesh objects
     * 
     * Manages vertex and index buffers for rendering geometry.
     * Provides functionality for binding and drawing meshes with Vulkan.
     */
    class Mesh
    {
    public:

        /**
         * @enum InputRate
         * @brief Specifies the input rate for vertex attributes
         */
        enum class InputRate
        {
            VERTEX = 0,      ///< Per-vertex input rate
            INSTANCE = 1,    ///< Per-instance input rate
            UNDEFINED = 0x7FFFFFFF  ///< Invalid input rate
        };

        /**
         * @struct VertexAttributeDescription
         * @brief Describes a single vertex attribute
         */
        struct VertexAttributeDescription
        {
            uint32_t Location;                  ///< Shader attribute location
            uint32_t Binding;                   ///< Vertex buffer binding index
            VulkanHelper::Format Format;        ///< Data format of the attribute
            uint32_t Offset;                    ///< Offset within the vertex structure
        };

        /**
         * @struct VertexBindingDescription
         * @brief Describes how vertex data is bound and consumed
         */
        struct VertexBindingDescription
        {
            uint32_t Binding;                   ///< Binding index
            uint32_t Stride;                    ///< Size of each vertex in bytes
            InputRate PerInstance;              ///< Whether data is per-vertex or per-instance
        };

        /**
         * @struct Config
         * @brief Configuration parameters for creating a mesh
         */
        struct Config
        {
            /**
             * @brief Device to create the mesh on
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device = nullptr;
            
            /**
             * @brief Command buffer for uploading mesh data
             * @note Must not be nullptr and must be in recording state
             */
            VulkanHelper::CommandBuffer* CommandBuffer = nullptr;

            /**
             * @brief Array of vertex attribute formats
             * @note Must not be nullptr and must have VertexAttributeCount elements
             */
            VulkanHelper::Format* VertexAttributes = nullptr;
            
            /**
             * @brief Number of vertex attributes
             * @note Must be greater than 0
             */
            uint32_t VertexAttributeCount = 0;

            /**
             * @brief Pointer to vertex data to upload
             * @note Must not be nullptr and must have VertexDataSize bytes
             */
            void* VertexData = nullptr;
            
            /**
             * @brief Size of vertex data in bytes
             * @note Must be greater than 0 and evenly divisible by vertex size
             */
            uint32_t VertexDataSize = 0;

            /**
             * @brief Pointer to index data to upload (optional)
             * @note Can be nullptr if no indices are used
             */
            void* IndexData = nullptr;
            
            /**
             * @brief Size of index data in bytes (optional)
             * @note Can be 0 if no indices are used, otherwise must be divisible by sizeof(uint32_t)
             */
            uint32_t IndexDataSize = 0;
        };

        /**
         * @brief Creates a new mesh with vertex and optional index data
         * @param config Configuration parameters for the mesh
         * @return Expected containing the created mesh or an error code
         */
        [[nodiscard]] static Expected<Mesh, VHResult> New(const Config& config);

        /**
         * @brief Delete copy constructor
         */
        Mesh(const Mesh& other) = delete;
        
        /**
         * @brief Delete copy assignment operator
         */
        Mesh& operator=(const Mesh& other) = delete;
        
        /**
         * @brief Move constructor
         */
        Mesh(Mesh&& other) noexcept;
        
        /**
         * @brief Move assignment operator
         */
        Mesh& operator=(Mesh&& other) noexcept;

        /**
         * @brief Destructor
         */
        ~Mesh();

        /**
         * @brief Bind vertex and index buffers to a command buffer
         * @param commandBuffer Command buffer to bind buffers to
         * @note Command buffer must be in recording state
         */
        void Bind(CommandBuffer* commandBuffer) const;
        
        /**
         * @brief Draw the mesh using the bound buffers
         * @param commandBuffer Command buffer to record draw commands
         * @param instanceCount Number of instances to draw
         * @param firstInstance First instance index to draw
         * @note Command buffer must be in recording state and mesh must be bound
         */
        void Draw(CommandBuffer* commandBuffer, uint32_t instanceCount = 1, uint32_t firstInstance = 0) const;

        /**
         * @brief Get vertex attribute descriptions for the mesh
         * @return Vector of vertex attribute descriptions
         */
        [[nodiscard]] const VulkanHelper::Vector<VertexAttributeDescription>& GetAttributesDescriptions() const;
        
        /**
         * @brief Get vertex binding description for the mesh
         * @return Vertex binding description
         */
        [[nodiscard]] VertexBindingDescription GetBindingDescription() const;

        class Impl;
    private:
        friend class Impl;
        UniquePtr<Impl> m_Impl;
        Mesh(UniquePtr<Impl>&& impl);
    };
}