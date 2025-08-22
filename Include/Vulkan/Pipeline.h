#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/SharedPtr.h"
#include "Core/Vector.h"

#include "Vulkan/Device.h"
#include "Vulkan/Shader.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/PushConstant.h"
#include "Vulkan/DescriptorSet.h"

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
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Shader stages used in this pipeline
             */
            VulkanHelper::Vector<Shader> Shaders;

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

            /**
             * @brief Descriptor sets to bind with this pipeline (Optional)
             * 
             * @note If not empty, descriptor sets will be automatically bound when the pipeline is bound
             * @note All descriptor sets must outlive this object
             */
            VulkanHelper::Vector<DescriptorSet> DescriptorSets;

            /**
             * @brief Push constant (Optional)
             * 
             * @note If not nullptr, must outlive this object
             */
			VulkanHelper::PushConstant* PushConstant = nullptr;

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
            /**
             * @brief The logical device that will own this pipeline
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Shader used in the pipeline
             * 
             * @note Must be a valid shader
             */
            VulkanHelper::Shader ComputeShader{};

            /**
             * @brief Descriptor sets to bind with this pipeline (Optional)
             * 
             * @note If not empty, descriptor sets will be automatically bound when the pipeline is bound
             * @note All descriptor sets must outlive this object
             */
            VulkanHelper::Vector<DescriptorSet> DescriptorSets;

            /**
             * @brief Push constant (Optional)
             * 
             * @note If not nullptr, must outlive this object
             */
			VulkanHelper::PushConstant* PushConstant = nullptr;
        };

        /**
         * @brief Configuration for ray tracing pipeline creation
         */
        struct RayTracingConfig
        {
            /**
             * @brief The logical device that will own this pipeline
             * @note Must be a valid device
             */
            VulkanHelper::Device Device{};

            /**
             * @brief Ray Generation shaders
             * 
             * @note Must not be empty
             */
            VulkanHelper::Vector<Shader> RayGenShaders;

            /**
             * @brief Miss shaders
             * 
             * @note Must outlive this object
             */
            VulkanHelper::Vector<Shader> MissShaders;

            /**
             * @brief Hit shaders
             * 
             * @note Must outlive this object
             */
            VulkanHelper::Vector<Shader> HitShaders;

            /**
             * @brief Descriptor sets to bind with this pipeline (Optional)
             * 
             * @note If not empty, descriptor sets will be automatically bound when the pipeline is bound
             * @note All descriptor sets must outlive this object
             */
            VulkanHelper::Vector<DescriptorSet> DescriptorSets;

            /**
             * @brief Push constant (Optional)
             * 
             * @note If not nullptr, must outlive this object
             */
			VulkanHelper::PushConstant* PushConstant = nullptr;

            /**
             * @brief Command buffer used for SBT creation
             * 
             * @note Must not be nullptr and must outlive this object
             */
            VulkanHelper::CommandBuffer* CommandBuffer = nullptr;
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

        Pipeline();

        Pipeline(const Pipeline& other);
        Pipeline& operator=(const Pipeline& other);

        Pipeline(Pipeline&& other) noexcept;
        Pipeline& operator=(Pipeline&& other) noexcept;

        ~Pipeline();

        /**
         * @brief Binds this pipeline to a command buffer for rendering.
         * 
         * @param commandBuffer The command buffer to bind this pipeline to
         */
        void Bind(CommandBuffer& commandBuffer);

        /**
         * @brief Dispatches a compute shader with the specified group counts.
         * 
         * @param commandBuffer The command buffer to dispatch the compute shader on
         * @param groupCountX Number of workgroups in the X dimension
         * @param groupCountY Number of workgroups in the Y dimension
         * @param groupCountZ Number of workgroups in the Z dimension
         * 
         * @note Available only for compute pipelines
         */
        void Dispatch(CommandBuffer& commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

        /**
         * @brief Dispatches a ray tracing shader with the specified dimensions.
         * 
         * @param commandBuffer The command buffer to dispatch the ray tracing shader on
         * @param width Width of the ray tracing dispatch
         * @param height Height of the ray tracing dispatch
         * @param depth Depth of the ray tracing dispatch (default is 1)
         * 
         * @note Available only for ray tracing pipelines
         */
        void RayTrace(CommandBuffer& commandBuffer, uint32_t width, uint32_t height, uint32_t depth = 1);

        /**
         * @brief Pushes the current push constant data to the specified command buffer.
         * 
         * @param commandBuffer The command buffer to push the constants to
         */
        void PushConstants(CommandBuffer& commandBuffer);

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        Pipeline(const SharedPtr<Impl>& impl);
    };
}