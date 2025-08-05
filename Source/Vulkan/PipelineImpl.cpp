#include "PipelineImpl.h"
#include "ShaderImpl.h"
#include "CommandBufferImpl.h"
#include "PushConstantImpl.h"

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Pipeline::Impl>, VHResult> Pipeline::Impl::New(const GraphicsConfig& config)
    {
        VH_LOG_INFO("Creating Graphics Pipeline Implementation");

        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Device Cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // TODO descriptors sets

        // Handle push constants
        VkPushConstantRange pushConstantRange{};
        if (config.PushConstant != nullptr)
        {
            PushConstant::Impl* pushConstantImpl = PushConstant::Impl::GetImplementation(config.PushConstant);
            pushConstantRange = pushConstantImpl->GetVkPushConstantRange();
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &pushConstantRange;
        }
        else
        {
            layoutInfo.pushConstantRangeCount = 0;
            layoutInfo.pPushConstantRanges = nullptr;
        }

        VkPipelineLayout pipelineLayout;
        Device::Impl* deviceImpl = Device::Impl::GetImplementation(config.Device);
        VkResult res = vkCreatePipelineLayout(deviceImpl->GetDevice(), &layoutInfo, nullptr, &pipelineLayout);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Couldn't create pipeline layout!");
            return Unexpected((VHResult)res);
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = (VkPrimitiveTopology)config.Topology;
		if (config.Topology == PrimitiveTopology::LINE_STRIP || config.Topology == PrimitiveTopology::TRIANGLE_STRIP)
            inputAssemblyInfo.primitiveRestartEnable = VK_TRUE;
		else
            inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.depthClampEnable = config.DepthClamp;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode = (VkPolygonMode)config.PolygonMode;
		rasterizationInfo.lineWidth = 1.0f;
		rasterizationInfo.cullMode = (VkCullModeFlags)config.CullMode;
		rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.rasterizationSamples = (VkSampleCountFlagBits)config.SampleCount;
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.pSampleMask = nullptr;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

        VulkanHelper::Vector<VkPipelineColorBlendAttachmentState> blendStates(config.ColorAttachmentCount);
        for (size_t i = 0; i < config.ColorAttachmentCount; i++)
        {
            if (config.BlendingEnable)
			{
				auto& blendAttachment = blendStates[i];
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
				auto& blendAttachment = blendStates[i];
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

        VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
        colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendInfo.logicOpEnable = VK_FALSE;
		colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendInfo.attachmentCount = config.ColorAttachmentCount;
		colorBlendInfo.pAttachments = blendStates.Data();
		colorBlendInfo.blendConstants[0] = 0.0f;
		colorBlendInfo.blendConstants[1] = 0.0f;
		colorBlendInfo.blendConstants[2] = 0.0f;
		colorBlendInfo.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = config.DepthTestEnable;
		depthStencilInfo.depthWriteEnable = VK_TRUE;
		depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilInfo.minDepthBounds = 0.0f;
		depthStencilInfo.maxDepthBounds = 1.0f;
		depthStencilInfo.stencilTestEnable = VK_FALSE;
		depthStencilInfo.front = {};
		depthStencilInfo.back = {};

        VkDynamicState dynamicStates[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

        VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.dynamicStateCount = 2;
		dynamicStateInfo.pDynamicStates = dynamicStates;

        VkVertexInputBindingDescription bindingDesc = {};
        bindingDesc.binding = config.BindingDesc.Binding;
        bindingDesc.stride = config.BindingDesc.Stride;
        bindingDesc.inputRate = (VkVertexInputRate)config.BindingDesc.PerInstance;

        VulkanHelper::Vector<VkVertexInputAttributeDescription> attributeDescs(config.AttributeDesc->Size());
        for (size_t i = 0; i < config.AttributeDesc->Size(); i++)
        {
            const Mesh::VertexAttributeDescription& attrDesc = (*config.AttributeDesc)[i];
            attributeDescs[i].location = attrDesc.Location;
            attributeDescs[i].binding = attrDesc.Binding;
            attributeDescs[i].format = (VkFormat)attrDesc.Format;
            attributeDescs[i].offset = attrDesc.Offset;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescs.Size();
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescs.Data();

        // This is a dynamic state, it's set properly when begining the rendering. This is just a placeholder since Vulkan requires me to pass something
        // in there for whatever reason
        VkViewport viewport{};
        viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)1;
		viewport.height = (float)1;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

        VkRect2D scissors{};
        scissors.extent = {1, 1};
        scissors.offset = {0, 0};

        VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;
		viewportInfo.pViewports = &viewport;
		viewportInfo.scissorCount = 1;
		viewportInfo.pScissors = &scissors;

        VulkanHelper::Vector<VkPipelineShaderStageCreateInfo> shaderStages(config.Shaders.Size());
        for (size_t i = 0; i < shaderStages.Size(); i++)
        {
            Shader::Impl* shaderImpl = Shader::Impl::GetImplementation(config.Shaders[i]);
            shaderStages[i] = shaderImpl->GetShaderStageCreateInfo();
        }

        VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
        graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineInfo.stageCount = (uint32_t)shaderStages.Size();
		graphicsPipelineInfo.pStages = shaderStages.Data();
		graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
		graphicsPipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
		graphicsPipelineInfo.pViewportState = &viewportInfo;
		graphicsPipelineInfo.pRasterizationState = &rasterizationInfo;
		graphicsPipelineInfo.pMultisampleState = &multisampleInfo;
		graphicsPipelineInfo.pColorBlendState = &colorBlendInfo;
		graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
		graphicsPipelineInfo.pDepthStencilState = &depthStencilInfo;
		graphicsPipelineInfo.layout = pipelineLayout;

        // VH uses Dynamic Rendering
		graphicsPipelineInfo.renderPass = VK_NULL_HANDLE; 
		graphicsPipelineInfo.subpass = 0;

        VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
        pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
		pipelineRenderingInfo.pNext = VK_NULL_HANDLE;
		pipelineRenderingInfo.colorAttachmentCount = config.ColorAttachmentCount;
		pipelineRenderingInfo.pColorAttachmentFormats = (VkFormat*)config.ColorFormats.Data();
		pipelineRenderingInfo.depthAttachmentFormat = (VkFormat)config.DepthFormat;
		pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
		graphicsPipelineInfo.pNext = &pipelineRenderingInfo;

		graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		graphicsPipelineInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        res = vkCreateGraphicsPipelines(deviceImpl->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
            return Unexpected((VHResult)res);

        PushConstant::Impl* pushConstantImpl = nullptr;
        if (config.PushConstant != nullptr)
            pushConstantImpl = PushConstant::Impl::GetImplementation(config.PushConstant);

        return UniquePtr(new Impl(deviceImpl, pipeline, pipelineLayout, Pipeline::PipelineType::Graphics, pushConstantImpl));
    }

    VulkanHelper::Expected<VulkanHelper::UniquePtr<Pipeline::Impl>, VHResult> Pipeline::Impl::New(const ComputeConfig& config)
    {
        VH_LOG_INFO("Creating Compute Pipeline Implementation");

        (void)config;
        return Unexpected(VHResult::NOT_IMPLEMENTED);
    }

    VulkanHelper::Expected<VulkanHelper::UniquePtr<Pipeline::Impl>, VHResult> Pipeline::Impl::New(const RayTracingConfig& config)
    {
        VH_LOG_INFO("Creating RayTracing Pipeline Implementation");

        (void)config;
        return Unexpected(VHResult::NOT_IMPLEMENTED);
    }

    Pipeline::Impl::~Impl()
    {
        if (m_Pipeline != nullptr)
        {
            VH_LOG_INFO("Destroying Pipeline Implementation");
            vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
            vkDestroyPipelineLayout(m_Device->GetDevice(), m_Layout, nullptr);
            m_Device = nullptr;
            m_Pipeline = nullptr;
            m_Layout = nullptr;
        }
    }

    Pipeline::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Pipeline(other.m_Pipeline)
        , m_Layout(other.m_Layout)
        , m_PipelineType(other.m_PipelineType)
        , m_PushConstant(other.m_PushConstant)
    {
        other.m_Device = nullptr;
        other.m_Pipeline = nullptr;
        other.m_Layout = nullptr;
        other.m_PushConstant = nullptr;
    }

    Pipeline::Impl& Pipeline::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Cleanup current state

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Pipeline = other.m_Pipeline;
        other.m_Pipeline = nullptr;
        m_Layout = other.m_Layout;
        other.m_Layout = nullptr;
        m_PipelineType = other.m_PipelineType;
        m_PushConstant = other.m_PushConstant;
        other.m_PushConstant = nullptr;

        return *this;
    }

    void Pipeline::Impl::Bind(CommandBuffer* commandBuffer)
    {
        CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(commandBuffer);

        VkPipelineBindPoint bindPoint;
        switch (m_PipelineType)
        {
        case Pipeline::PipelineType::Graphics:
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            break;

        case Pipeline::PipelineType::Compute:
            bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            break;

        case Pipeline::PipelineType::RayTracing:
            bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
            break;
        
        default:
            VH_ASSERT(false, "Incorrect pipeline type! You should never see this unless the memory is corrupted.");
            return;
        }
        vkCmdBindPipeline(cmdImpl->GetCommandBuffer(), bindPoint, m_Pipeline);

        // Push constants after binding the pipeline
        if (m_PushConstant != nullptr)
        {
            VkPushConstantRange range = m_PushConstant->GetVkPushConstantRange();
            
            vkCmdPushConstants(
                cmdImpl->GetCommandBuffer(),
                m_Layout,
                range.stageFlags,
                range.offset,
                range.size,
                m_PushConstant->GetData()
            );
        }
    }

    //
    // Forward functions
    //
    
    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const GraphicsConfig& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Pipeline{ VulkanHelper::Move(implResult.Value()) };
    }

    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const ComputeConfig& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Pipeline{ VulkanHelper::Move(implResult.Value()) };
    }

    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const RayTracingConfig& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Pipeline{ VulkanHelper::Move(implResult.Value()) };
    }

    Pipeline::~Pipeline()
    {

    }

    Pipeline::Pipeline(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    Pipeline::Pipeline(Pipeline&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Pipeline& Pipeline::operator=(Pipeline&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Pipeline(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    void Pipeline::Bind(CommandBuffer* commandBuffer)
    {
        m_Impl->Bind(commandBuffer);
    }
}