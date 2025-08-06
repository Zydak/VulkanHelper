#include "PipelineImpl.h"
#include "ShaderImpl.h"
#include "CommandBufferImpl.h"
#include "PushConstantImpl.h"
#include "DescriptorSetImpl.h"

namespace VulkanHelper
{
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Pipeline::Impl>, VHResult> Pipeline::Impl::New(Device::Impl* device, Vector<Shader::Impl*> shaders, const Vector<Mesh::VertexAttributeDescription>* attributeDesc, Mesh::VertexBindingDescription bindingDesc, PolygonMode polygonMode, PrimitiveTopology topology, CullMode cullMode, bool depthTestEnable, bool depthClamp, bool blendingEnable, Vector<DescriptorSet::Impl*> descriptorSets, PushConstant::Impl* pushConstant, uint32_t colorAttachmentCount, Vector<Format> colorFormats, Format depthFormat, SampleCount sampleCount)
    {
        VH_LOG_INFO("Creating Graphics Pipeline Implementation");

        if (device == nullptr)
        {
            VH_LOG_ERROR("Device Cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        // Handle descriptor sets
        VulkanHelper::Vector<VkDescriptorSetLayout> descriptorSetLayouts;
        if (!descriptorSets.Empty())
        {
            descriptorSetLayouts.Reserve(descriptorSets.Size());
            for (size_t i = 0; i < descriptorSets.Size(); ++i)
            {
                descriptorSetLayouts.PushBack(descriptorSets[i]->GetDescriptorSetLayout());
            }
            layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.Size());
            layoutInfo.pSetLayouts = descriptorSetLayouts.Data();
        }
        else
        {
            layoutInfo.setLayoutCount = 0;
            layoutInfo.pSetLayouts = nullptr;
        }

        // Handle push constants
        VkPushConstantRange pushConstantRange{};
        if (pushConstant != nullptr)
        {
            pushConstantRange = pushConstant->GetVkPushConstantRange();
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &pushConstantRange;
        }
        else
        {
            layoutInfo.pushConstantRangeCount = 0;
            layoutInfo.pPushConstantRanges = nullptr;
        }

        VkPipelineLayout pipelineLayout;
        VkResult res = vkCreatePipelineLayout(device->GetDevice(), &layoutInfo, nullptr, &pipelineLayout);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Couldn't create pipeline layout!");
            return Unexpected((VHResult)res);
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyInfo.topology = (VkPrimitiveTopology)topology;
		if (topology == PrimitiveTopology::LINE_STRIP || topology == PrimitiveTopology::TRIANGLE_STRIP)
            inputAssemblyInfo.primitiveRestartEnable = VK_TRUE;
		else
            inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
        rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.depthClampEnable = depthClamp;
		rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationInfo.polygonMode = (VkPolygonMode)polygonMode;
		rasterizationInfo.lineWidth = 1.0f;
		rasterizationInfo.cullMode = (VkCullModeFlags)cullMode;
		rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizationInfo.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampleInfo{};
        multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleInfo.sampleShadingEnable = VK_FALSE;
		multisampleInfo.rasterizationSamples = (VkSampleCountFlagBits)sampleCount;
		multisampleInfo.minSampleShading = 1.0f;
		multisampleInfo.pSampleMask = nullptr;
		multisampleInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleInfo.alphaToOneEnable = VK_FALSE;

        VulkanHelper::Vector<VkPipelineColorBlendAttachmentState> blendStates(colorAttachmentCount);
        for (size_t i = 0; i < colorAttachmentCount; i++)
        {
            if (blendingEnable)
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
		colorBlendInfo.attachmentCount = colorAttachmentCount;
		colorBlendInfo.pAttachments = blendStates.Data();
		colorBlendInfo.blendConstants[0] = 0.0f;
		colorBlendInfo.blendConstants[1] = 0.0f;
		colorBlendInfo.blendConstants[2] = 0.0f;
		colorBlendInfo.blendConstants[3] = 0.0f;

        VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
        depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilInfo.depthTestEnable = depthTestEnable;
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

        VkVertexInputBindingDescription bindingDescVk = {};
        bindingDescVk.binding = bindingDesc.Binding;
        bindingDescVk.stride = bindingDesc.Stride;
        bindingDescVk.inputRate = (VkVertexInputRate)bindingDesc.InputRate;

        VulkanHelper::Vector<VkVertexInputAttributeDescription> attributeDescs(attributeDesc->Size());
        for (size_t i = 0; i < attributeDesc->Size(); i++)
        {
            const Mesh::VertexAttributeDescription& attrDesc = (*attributeDesc)[i];
            attributeDescs[i].location = attrDesc.Location;
            attributeDescs[i].binding = attrDesc.Binding;
            attributeDescs[i].format = (VkFormat)attrDesc.Format;
            attributeDescs[i].offset = attrDesc.Offset;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescVk;
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

        VulkanHelper::Vector<VkPipelineShaderStageCreateInfo> shaderStages(shaders.Size());
        for (size_t i = 0; i < shaderStages.Size(); i++)
        {
            shaderStages[i] = shaders[i]->GetShaderStageCreateInfo();
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
		pipelineRenderingInfo.colorAttachmentCount = colorAttachmentCount;
		pipelineRenderingInfo.pColorAttachmentFormats = (VkFormat*)colorFormats.Data();
		pipelineRenderingInfo.depthAttachmentFormat = (VkFormat)depthFormat;
		pipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
		graphicsPipelineInfo.pNext = &pipelineRenderingInfo;

		graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		graphicsPipelineInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        res = vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
            return Unexpected((VHResult)res);

        return UniquePtr(new Impl(device, pipeline, pipelineLayout, Pipeline::PipelineType::Graphics, VulkanHelper::Move(descriptorSets), pushConstant));
    }

    VulkanHelper::Expected<VulkanHelper::UniquePtr<Pipeline::Impl>, VHResult> Pipeline::Impl::New(const ComputeConfig& config)
    {
        VH_LOG_INFO("Creating Compute Pipeline Implementation");

        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Device cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.ComputeShader == nullptr)
        {
            VH_LOG_ERROR("ComputeShader cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        Device::Impl* device = Device::Impl::GetImplementation(config.Device);
        Shader::Impl* computeShader = Shader::Impl::GetImplementation(config.ComputeShader);

        // Validate that the shader is indeed a compute shader
        if (computeShader->GetShaderStage() != VK_SHADER_STAGE_COMPUTE_BIT)
        {
            VH_LOG_ERROR("Provided shader is not a compute shader!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        // Handle descriptor sets
        VulkanHelper::Vector<VkDescriptorSetLayout> descriptorSetLayouts;
        VulkanHelper::Vector<DescriptorSet::Impl*> descriptorSets;
        if (!config.DescriptorSets.Empty())
        {
            descriptorSetLayouts.Reserve(config.DescriptorSets.Size());
            descriptorSets.Reserve(config.DescriptorSets.Size());
            for (size_t i = 0; i < config.DescriptorSets.Size(); ++i)
            {
                DescriptorSet::Impl* descriptorSetImpl = DescriptorSet::Impl::GetImplementation(config.DescriptorSets[i]);
                descriptorSetLayouts.PushBack(descriptorSetImpl->GetDescriptorSetLayout());
                descriptorSets.PushBack(descriptorSetImpl);
            }
            layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.Size());
            layoutInfo.pSetLayouts = descriptorSetLayouts.Data();
        }
        else
        {
            layoutInfo.setLayoutCount = 0;
            layoutInfo.pSetLayouts = nullptr;
        }

        // Handle push constants
        VkPushConstantRange pushConstantRange{};
        PushConstant::Impl* pushConstant = nullptr;
        if (config.PushConstant != nullptr)
        {
            pushConstant = PushConstant::Impl::GetImplementation(config.PushConstant);
            pushConstantRange = pushConstant->GetVkPushConstantRange();
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &pushConstantRange;
        }
        else
        {
            layoutInfo.pushConstantRangeCount = 0;
            layoutInfo.pPushConstantRanges = nullptr;
        }

        VkPipelineLayout pipelineLayout;
        VkResult res = vkCreatePipelineLayout(device->GetDevice(), &layoutInfo, nullptr, &pipelineLayout);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Couldn't create compute pipeline layout!");
            return Unexpected((VHResult)res);
        }

        // Create compute pipeline
        VkComputePipelineCreateInfo computePipelineInfo{};
        computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineInfo.stage = computeShader->GetShaderStageCreateInfo();
        computePipelineInfo.layout = pipelineLayout;
        computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        computePipelineInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        res = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Couldn't create compute pipeline!");
            vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
            return Unexpected((VHResult)res);
        }

        return UniquePtr(new Impl(device, pipeline, pipelineLayout, Pipeline::PipelineType::Compute, VulkanHelper::Move(descriptorSets), pushConstant));
    }

    VulkanHelper::Expected<VulkanHelper::UniquePtr<Pipeline::Impl>, VHResult> Pipeline::Impl::New(const RayTracingConfig& config)
    {
        VH_LOG_INFO("Creating RayTracing Pipeline Implementation");

        (void)config;
        return Unexpected(VHResult::NOT_IMPLEMENTED);
    }

    Pipeline::Impl::~Impl()
    {
        if (m_Pipeline != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying Pipeline Implementation");
            vkDestroyPipeline(m_Device->GetDevice(), m_Pipeline, nullptr);
            vkDestroyPipelineLayout(m_Device->GetDevice(), m_Layout, nullptr);
            m_Device = nullptr;
            m_Pipeline = VK_NULL_HANDLE;
            m_Layout = VK_NULL_HANDLE;
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
        other.m_Pipeline = VK_NULL_HANDLE;
        other.m_Layout = VK_NULL_HANDLE;
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
        other.m_Pipeline = VK_NULL_HANDLE;
        m_Layout = other.m_Layout;
        other.m_Layout = VK_NULL_HANDLE;
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

        // Bind descriptor sets if any
        if (!m_DescriptorSets.Empty())
        {
            VulkanHelper::Vector<VkDescriptorSet> descriptorSets;
            descriptorSets.Reserve(m_DescriptorSets.Size());
            
            for (size_t i = 0; i < m_DescriptorSets.Size(); ++i)
            {
                descriptorSets.PushBack(m_DescriptorSets[i]->GetDescriptorSet());
            }

            vkCmdBindDescriptorSets(
                cmdImpl->GetCommandBuffer(),
                bindPoint,
                m_Layout,
                0, // firstSet
                static_cast<uint32_t>(descriptorSets.Size()),
                descriptorSets.Data(),
                0, // dynamicOffsetCount
                nullptr // pDynamicOffsets
            );
        }

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

    void Pipeline::Impl::Dispatch(CommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        VH_ASSERT(m_PipelineType == Pipeline::PipelineType::Compute, "Dispatch can only be called on compute pipelines!");

        CommandBuffer::Impl* cmdImpl = CommandBuffer::Impl::GetImplementation(commandBuffer);
        vkCmdDispatch(cmdImpl->GetCommandBuffer(), groupCountX, groupCountY, groupCountZ);
    }

    //
    // Forward functions
    //
    
    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const GraphicsConfig& config)
    {
        VulkanHelper::Vector<Shader::Impl*> shaders(config.Shaders.Size());
        for (size_t i = 0; i < config.Shaders.Size(); i++)
            shaders[i] = Shader::Impl::GetImplementation(config.Shaders[i]);

        VulkanHelper::Vector<DescriptorSet::Impl*> descriptorSets;
        if (!config.DescriptorSets.Empty())
        {
            descriptorSets.Reserve(config.DescriptorSets.Size());
            for (size_t i = 0; i < config.DescriptorSets.Size(); i++)
                descriptorSets.PushBack(DescriptorSet::Impl::GetImplementation(config.DescriptorSets[i]));
        }

        PushConstant::Impl* pushConstant = nullptr;
        if (config.PushConstant != nullptr)
            pushConstant = PushConstant::Impl::GetImplementation(config.PushConstant);

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            shaders,
            config.AttributeDesc,
            config.BindingDesc,
            config.PolygonMode,
            config.Topology,
            config.CullMode,
            config.DepthTestEnable,
            config.DepthClamp,
            config.BlendingEnable,
            descriptorSets,
            pushConstant,
            config.ColorAttachmentCount,
            config.ColorFormats,
            config.DepthFormat,
            config.SampleCount
        );

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

    void Pipeline::Dispatch(CommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        m_Impl->Dispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
    }
}