#pragma once
#include "pch.h"

#include "Device.h"

#include "Shader.h"

namespace VulkanHelper
{
	class Pipeline
	{
	public:
		struct GraphicsCreateInfo;
		struct ComputeCreateInfo;
		struct RayTracingCreateInfo;

		void Destroy();

		Pipeline(const GraphicsCreateInfo& info);
		Pipeline(const ComputeCreateInfo& info);
		Pipeline(const RayTracingCreateInfo& info);
		~Pipeline();

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;
		Pipeline(Pipeline&& other) noexcept;
		Pipeline& operator=(Pipeline&& other) noexcept;
	public:

		void Bind(VkCommandBuffer commandBuffer) const;

		[[nodiscard]] inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
		[[nodiscard]] inline VkPipeline GetPipeline() const { return m_PipelineHandle; }

	public:
		struct GraphicsCreateInfo
		{
			Device* Device = nullptr;
			std::vector<Shader*> Shaders;
			std::vector<VkVertexInputBindingDescription> BindingDesc;
			std::vector<VkVertexInputAttributeDescription> AttributeDesc;
			VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
			uint32_t Width = 0;
			uint32_t Height = 0;
			VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
			bool DepthTestEnable = false;
			bool DepthClamp = false;
			bool BlendingEnable = false;
			std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
			VkPushConstantRange* PushConstants = nullptr;
			uint32_t ColorAttachmentCount = 1;
			std::vector<VkFormat> ColorFormats;
			VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

			const char* debugName = "";
		};

		struct ComputeCreateInfo
		{
			Device* Device = nullptr;
			VulkanHelper::Shader* Shader = nullptr;
			std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;
			VkPushConstantRange* PushConstants = nullptr;

			const char* debugName = "";
		};

		struct RayTracingCreateInfo
		{
			Device* Device = nullptr;
			std::vector<Shader*> RayGenShaders;
			std::vector<Shader*> MissShaders;
			std::vector<Shader*> HitShaders;
			VkPushConstantRange* PushConstants = nullptr;
			std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;

			const char* debugName = "";
		};

	private:

		struct PipelineConfigInfo
		{
			VkViewport Viewport = {};
			VkRect2D Scissor = { 0, 0 };
			VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo = {};
			VkPipelineRasterizationStateCreateInfo RasterizationInfo = {};
			VkPipelineMultisampleStateCreateInfo MultisampleInfo = {};
			std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachment;
			VkPipelineColorBlendStateCreateInfo ColorBlendInfo = {};
			VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
			VkPipelineLayout PipelineLayout = 0;
			uint32_t Subpass = 0;
			bool DepthClamp = false;
		};

		void CreatePipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t width, uint32_t height, VkPolygonMode polyMode, VkPrimitiveTopology topology, VkCullModeFlags cullMode, bool depthTestEnable, bool blendingEnable, int colorAttachmentCount = 1);

		void CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetsLayouts, VkPushConstantRange* pushConstants);

		enum class PipelineType
		{
			Graphics,
			Compute,
			RayTracing,
			Undefined
		};

		Device* m_Device = nullptr;
		VkPipeline m_PipelineHandle = 0;
		VkPipelineLayout m_PipelineLayout = 0;
		PipelineType m_PipelineType = PipelineType::Undefined;

		void Move(Pipeline&& other) noexcept;
	};

}