#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"

#include "Vulkan/Device.h"
#include "Vulkan/Shader.h"
#include "Vulkan/CommandBuffer.h"

#include "Renderer/Mesh.h"

namespace VulkanHelper
{
    /**
     * @class Pipeline
     * @brief RAII wrapper for a Vulkan graphics, compute, or ray tracing pipeline. Manages pipeline state and binding.
     */
    class Pipeline
    {
    public:
        /**
         * @brief Configuration for graphics pipeline creation
         */
        struct GraphicsConfig
        {
            /**
             * @brief The logical device that will own this pipeline
             * 
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::Device* Device;

            /**
             * @brief Shader stages used in this pipeline
             * @note Shaders must not be nullptrs and outlive this object
             */
            VulkanHelper::Vector<Shader*> Shaders;

            /**
             * @brief Vertex input state configuration
             * @note Must not be nullptr
             */
			const VulkanHelper::Vector<Mesh::VertexAttributeDescription>* AttributeDesc;
			
            /**
             * @brief Vertex binding description
             */
            VulkanHelper::Mesh::VertexBindingDescription BindingDesc;

            /**
             * @brief How polygons should be rasterized
             */
			VulkanHelper::PolygonMode PolygonMode = PolygonMode::FILL;

            /**
             * @brief The type of geometry to render
             */
			VulkanHelper::PrimitiveTopology Topology = VulkanHelper::PrimitiveTopology::TRIANGLE_LIST;

            /**
             * @brief Face culling mode
             */
			VulkanHelper::CullMode CullMode = VulkanHelper::CullMode::NONE;

            /**
             * @brief Whether to enable depth testing
             */
			bool DepthTestEnable = false;

            /**
             * @brief Whether to clamp depth values instead of clipping
             */
			bool DepthClamp = false;

            /**
             * @brief Whether to enable alpha blending
             */
			bool BlendingEnable = false;

			//VulkanHelper::Vector<VkDescriptorSetLayout> DescriptorSetLayouts; // TODO
			//VkPushConstantRange* PushConstants = nullptr; TODO

            /**
             * @brief Number of color attachments
             */
			uint32_t ColorAttachmentCount = 1;

            /**
             * @brief Format of each color attachment
             */
			VulkanHelper::Vector<VulkanHelper::Format> ColorFormats;

            /**
             * @brief Format of the depth attachment
             */
			VulkanHelper::Format DepthFormat = VulkanHelper::Format::D32_SFLOAT;

            /**
             * @brief MSAA sample count
             */
            VulkanHelper::SampleCount SampleCount = SampleCount::COUNT_1_BIT;
        };

        /**
         * @brief Configuration for compute pipeline creation
         */
        struct ComputeConfig
        {
            // TODO: Add compute pipeline configuration
        };

        /**
         * @brief Configuration for ray tracing pipeline creation
         */
        struct RayTracingConfig
        {
            // TODO: Add ray tracing pipeline configuration
        };

        /**
         * @brief Types of supported Vulkan pipelines
         */
        enum class PipelineType
        {
            Graphics,    ///< Graphics rendering pipeline
            Compute,     ///< Compute shader pipeline
            RayTracing   ///< Ray tracing pipeline
        };

        /**
         * @brief Creates a new graphics pipeline with the specified configuration.
         * 
         * @param config Graphics pipeline configuration
         * @return Expected<Pipeline, VHResult> The created pipeline or error code
         * @note Device must be valid and shaders must be compiled
         */
        [[nodiscard]] static Expected<Pipeline, VHResult> New(const GraphicsConfig& config);

        /**
         * @brief Creates a new compute pipeline with the specified configuration.
         * 
         * @param config Compute pipeline configuration
         * @return Expected<Pipeline, VHResult> The created pipeline or error code
         * @note Device must be valid and compute shader must be compiled
         */
        [[nodiscard]] static Expected<Pipeline, VHResult> New(const ComputeConfig& config);

        /**
         * @brief Creates a new ray tracing pipeline with the specified configuration.
         * 
         * @param config Ray tracing pipeline configuration
         * @return Expected<Pipeline, VHResult> The created pipeline or error code
         * @note Device must support ray tracing and shaders must be compiled
         */
        [[nodiscard]] static Expected<Pipeline, VHResult> New(const RayTracingConfig& config);

        Pipeline(const Pipeline& other) = delete;
        Pipeline& operator=(const Pipeline& other) = delete;

        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        ~Pipeline();

        /**
         * @brief Binds this pipeline to a command buffer for rendering.
         * 
         * @param commandBuffer The command buffer to bind this pipeline to
         */
        void Bind(CommandBuffer* commandBuffer);

        class Impl;
    private:
        friend class Impl;
        UniquePtr<Impl> m_Impl;

        Pipeline(UniquePtr<Impl>&& impl);
    };
}