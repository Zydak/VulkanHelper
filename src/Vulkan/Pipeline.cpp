#include "Pch.h"

#include "Pipeline.h"
#include "Logger/Logger.h"
#include "LoadedFunctions.h"

namespace VulkanHelper
{
	enum class ShaderType
	{
		Vertex,
		Fragment,
		Compute
	};

	struct ShaderModule
	{
		VkShaderModule Module;
		VkShaderStageFlagBits Type;
	};

	void Pipeline::Destroy()
	{
		// TODO: figure out why the fuck I can't have 2 ray tracing pipelines at the same time
		//if (m_PipelineType == PipelineType::RayTracing)
		if (m_PipelineHandle != VK_NULL_HANDLE)
		{
			m_Device->WaitUntilIdle();
			vkDestroyPipeline(m_Device->GetHandle(), m_PipelineHandle, nullptr);
			m_PipelineHandle = VK_NULL_HANDLE;

			vkDestroyPipelineLayout(m_Device->GetHandle(), m_PipelineLayout, nullptr);
		}
		//else
		//	DeleteQueue::TrashPipeline(*this);
	}

	Pipeline::~Pipeline()
	{
		Destroy();
	}

	Pipeline::Pipeline(const GraphicsCreateInfo& info)
	{
		m_Device = info.Device;
		m_PipelineType = PipelineType::Graphics;

		CreatePipelineLayout(info.DescriptorSetLayouts, info.PushConstants);
		PipelineConfigInfo configInfo{};
		configInfo.DepthClamp = info.DepthClamp;
		CreatePipelineConfigInfo(configInfo, info.Width, info.Height, info.PolygonMode, info.Topology, info.CullMode, info.DepthTestEnable, info.BlendingEnable, info.ColorAttachmentCount);

		std::vector<ShaderModule> shaderModules;
		shaderModules.resize(info.Shaders.size());
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (int i = 0; i < info.Shaders.size(); i++)
		{
			shaderStages.emplace_back(info.Shaders[i]->GetStageCreateInfo());
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)info.AttributeDesc.size();
		vertexInputInfo.pVertexAttributeDescriptions = info.AttributeDesc.data();
		vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)info.BindingDesc.size();
		vertexInputInfo.pVertexBindingDescriptions = info.BindingDesc.data();

		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &configInfo.Viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &configInfo.Scissor;

		VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = 2;
		dynamicStateInfo.pDynamicStates = dynamicStates;

		VkGraphicsPipelineCreateInfo graphicsPipelineInfo = {};

		graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineInfo.stageCount = (uint32_t)shaderStages.size();
		graphicsPipelineInfo.pStages = shaderStages.data();
		graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
		graphicsPipelineInfo.pInputAssemblyState = &configInfo.InputAssemblyInfo;
		graphicsPipelineInfo.pViewportState = &viewportInfo;
		graphicsPipelineInfo.pRasterizationState = &configInfo.RasterizationInfo;
		graphicsPipelineInfo.pMultisampleState = &configInfo.MultisampleInfo;
		graphicsPipelineInfo.pColorBlendState = &configInfo.ColorBlendInfo;
		graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
		graphicsPipelineInfo.pDepthStencilState = &configInfo.DepthStencilInfo;

		graphicsPipelineInfo.layout = m_PipelineLayout;
		graphicsPipelineInfo.renderPass = VK_NULL_HANDLE;
		graphicsPipelineInfo.subpass = configInfo.Subpass;

		VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
		pipelineRenderingInfo.pNext = VK_NULL_HANDLE;
		pipelineRenderingInfo.colorAttachmentCount = info.ColorAttachmentCount;
		pipelineRenderingInfo.pColorAttachmentFormats = info.ColorFormats.data();
		pipelineRenderingInfo.depthAttachmentFormat = info.DepthFormat;
		pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		graphicsPipelineInfo.pNext = &pipelineRenderingInfo;

		graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		graphicsPipelineInfo.basePipelineIndex = -1;

		VH_CHECK(
			vkCreateGraphicsPipelines(m_Device->GetHandle(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &m_PipelineHandle) == VK_SUCCESS,
			"failed to create graphics pipeline!"
		);

		if (std::string(info.debugName) != std::string())
		{
			m_Device->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineHandle, info.debugName);
		}
	}

	Pipeline::Pipeline(const ComputeCreateInfo& info)
	{
		m_Device = info.Device;

		m_PipelineType = PipelineType::Compute;

		CreatePipelineLayout(info.DescriptorSetLayouts, info.PushConstants);

		VkComputePipelineCreateInfo computePipelineInfo = {};

		computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineInfo.layout = m_PipelineLayout;
		computePipelineInfo.stage = info.Shader->GetStageCreateInfo();

		VH_CHECK(
			vkCreateComputePipelines(m_Device->GetHandle(), VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_PipelineHandle) == VK_SUCCESS,
			"failed to create graphics pipeline!"
		);

		if (std::string(info.debugName) != std::string())
		{
			m_Device->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineHandle, info.debugName);
		}
	}

	Pipeline::Pipeline(const RayTracingCreateInfo& info)
	{
		m_Device = info.Device;

		m_PipelineType = PipelineType::RayTracing;

		// All stages
		int count = (int)info.RayGenShaders.size() + (int)info.MissShaders.size() + (int)info.HitShaders.size();
		std::vector<VkPipelineShaderStageCreateInfo> stages(count);
		VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stage.pName = "main";  // All have the same entry point

		int stageCount = 0;

		// Ray gen
		for (int i = 0; i < info.RayGenShaders.size(); i++)
		{
			stage.module = info.RayGenShaders[i]->GetModuleHandle();
			stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			stages[stageCount] = stage;
			stageCount++;
		}

		// Miss
		for (int i = 0; i < info.MissShaders.size(); i++)
		{
			stage.module = info.MissShaders[i]->GetModuleHandle();
			stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			stages[stageCount] = stage;
			stageCount++;
		}

		// Hit Group - Closest Hit
		for (int i = 0; i < info.HitShaders.size(); i++)
		{
			stage.module = info.HitShaders[i]->GetModuleHandle();
			stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			stages[stageCount] = stage;
			stageCount++;
		}

		// Shader groups
		VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
		group.anyHitShader = VK_SHADER_UNUSED_KHR;
		group.closestHitShader = VK_SHADER_UNUSED_KHR;
		group.generalShader = VK_SHADER_UNUSED_KHR;
		group.intersectionShader = VK_SHADER_UNUSED_KHR;

		// Ray gen
		stageCount = 0;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> rtShaderGroups;
		for (int i = 0; i < info.RayGenShaders.size(); i++)
		{
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = stageCount;
			rtShaderGroups.push_back(group);
			stageCount++;
		}

		// Miss
		for (int i = 0; i < info.MissShaders.size(); i++)
		{
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			group.generalShader = stageCount;
			rtShaderGroups.push_back(group);
			stageCount++;
		}

		group.generalShader = VK_SHADER_UNUSED_KHR;

		// closest hit shader
		for (int i = 0; i < info.HitShaders.size(); i++)
		{
			group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			group.closestHitShader = stageCount;
			rtShaderGroups.push_back(group);
			stageCount++;
		}

		// create layout
		CreatePipelineLayout(info.DescriptorSetLayouts, info.PushConstants);

		// Assemble the shader stages and recursion depth info into the ray tracing pipeline
		VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
		rayPipelineInfo.stageCount = (uint32_t)stages.size();  // Stages are shaders
		rayPipelineInfo.pStages = stages.data();

		rayPipelineInfo.groupCount = (uint32_t)rtShaderGroups.size();
		rayPipelineInfo.pGroups = rtShaderGroups.data();

		rayPipelineInfo.maxPipelineRayRecursionDepth = 2;  // Ray depth
		rayPipelineInfo.layout = m_PipelineLayout;

		VulkanHelper::vkCreateRayTracingPipelinesKHR(m_Device->GetHandle(), {}, {}, 1, &rayPipelineInfo, nullptr, &m_PipelineHandle);

		if (std::string(info.debugName) != std::string())
		{
			m_Device->SetObjectName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineHandle, info.debugName);
		}
	}

	Pipeline::Pipeline(Pipeline&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	void Pipeline::Bind(VkCommandBuffer commandBuffer) const
	{
		VkPipelineBindPoint bindPoint;

		switch (m_PipelineType)
		{
		case VulkanHelper::Pipeline::PipelineType::Graphics:
			bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			break;
		case VulkanHelper::Pipeline::PipelineType::Compute:
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
			break;
		case VulkanHelper::Pipeline::PipelineType::RayTracing:
			bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			break;
		default:
			VH_CHECK(false, "Undefined Pipeline Type!");
			break;
		}

		vkCmdBindPipeline(commandBuffer, bindPoint, m_PipelineHandle);
	}

	void Pipeline::CreatePipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetsLayouts, VkPushConstantRange* pushConstants)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = descriptorSetsLayouts.empty() ? 0 : (uint32_t)descriptorSetsLayouts.size();
		pipelineLayoutInfo.pSetLayouts = descriptorSetsLayouts.empty() ? nullptr : descriptorSetsLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = (pushConstants == nullptr) ? 0 : 1;
		pipelineLayoutInfo.pPushConstantRanges = pushConstants;
		VH_CHECK(vkCreatePipelineLayout(m_Device->GetHandle(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) == VK_SUCCESS,
			"failed to create pipeline layout!"
		);
	}

	void Pipeline::CreatePipelineConfigInfo(PipelineConfigInfo& configInfo, uint32_t width, uint32_t height, VkPolygonMode polyMode, VkPrimitiveTopology topology, VkCullModeFlags cullMode, bool depthTestEnable, bool blendingEnable, int colorAttachmentCount)
	{
		configInfo.InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		configInfo.InputAssemblyInfo.topology = topology;
		if (topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP || topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP) configInfo.InputAssemblyInfo.primitiveRestartEnable = VK_TRUE;
		else configInfo.InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

		configInfo.Viewport.x = 0.0f;
		configInfo.Viewport.y = 0.0f;
		configInfo.Viewport.width = (float)width;
		configInfo.Viewport.height = (float)height;
		configInfo.Viewport.minDepth = 0.0f;
		configInfo.Viewport.maxDepth = 1.0f;

		configInfo.Scissor.offset = { 0, 0 };
		configInfo.Scissor.extent = { width, height };

		configInfo.RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		configInfo.RasterizationInfo.depthClampEnable = configInfo.DepthClamp;
		configInfo.RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		configInfo.RasterizationInfo.polygonMode = polyMode;
		configInfo.RasterizationInfo.lineWidth = 1.0f;
		configInfo.RasterizationInfo.cullMode = cullMode;
		configInfo.RasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		configInfo.RasterizationInfo.depthBiasEnable = VK_FALSE;

		configInfo.MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		configInfo.MultisampleInfo.sampleShadingEnable = VK_FALSE;
		configInfo.MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		configInfo.MultisampleInfo.minSampleShading = 1.0f;
		configInfo.MultisampleInfo.pSampleMask = nullptr;
		configInfo.MultisampleInfo.alphaToCoverageEnable = VK_FALSE;
		configInfo.MultisampleInfo.alphaToOneEnable = VK_FALSE;

		configInfo.ColorBlendAttachment.resize(colorAttachmentCount);
		for (int i = 0; i < colorAttachmentCount; i++)
		{
			if (blendingEnable)
			{
				auto& blendAttachment = configInfo.ColorBlendAttachment[i];
				blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				blendAttachment.blendEnable = VK_TRUE;
				blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			}
			else
			{
				auto& blendAttachment = configInfo.ColorBlendAttachment[i];
				blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				blendAttachment.blendEnable = VK_FALSE;
				blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
			}
		}

		configInfo.ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		configInfo.ColorBlendInfo.logicOpEnable = VK_FALSE;
		configInfo.ColorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		configInfo.ColorBlendInfo.attachmentCount = colorAttachmentCount;
		configInfo.ColorBlendInfo.pAttachments = configInfo.ColorBlendAttachment.data();
		configInfo.ColorBlendInfo.blendConstants[0] = 0.0f;
		configInfo.ColorBlendInfo.blendConstants[1] = 0.0f;
		configInfo.ColorBlendInfo.blendConstants[2] = 0.0f;
		configInfo.ColorBlendInfo.blendConstants[3] = 0.0f;

		configInfo.DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		configInfo.DepthStencilInfo.depthTestEnable = depthTestEnable;
		configInfo.DepthStencilInfo.depthWriteEnable = VK_TRUE;
		configInfo.DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		configInfo.DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		configInfo.DepthStencilInfo.minDepthBounds = 0.0f;
		configInfo.DepthStencilInfo.maxDepthBounds = 1.0f;
		configInfo.DepthStencilInfo.stencilTestEnable = VK_FALSE;
		configInfo.DepthStencilInfo.front = {};
		configInfo.DepthStencilInfo.back = {};
	}

	void Pipeline::Move(Pipeline&& other) noexcept
	{
		m_Device = other.m_Device;
		other.m_Device = nullptr;

		m_PipelineHandle = other.m_PipelineHandle;
		other.m_PipelineHandle = VK_NULL_HANDLE;

		m_PipelineLayout = other.m_PipelineLayout;
		other.m_PipelineLayout = VK_NULL_HANDLE;

		m_PipelineType = other.m_PipelineType;
		other.m_PipelineType = PipelineType::Undefined;
	}
}