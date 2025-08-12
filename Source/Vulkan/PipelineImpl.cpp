#include "PipelineImpl.h"
#include "ShaderImpl.h"
#include "CommandBufferImpl.h"
#include "PushConstantImpl.h"
#include "DescriptorSetImpl.h"

#include "FunctionLoader.h"

namespace VulkanHelper
{
    Expected<Pipeline::Impl, VHResult> Pipeline::Impl::New(
        Device::Impl* device,
        Vector<Shader::Impl*>&& shaders,
        const Vector<Mesh::VertexAttributeDescription>* attributeDesc,
        Mesh::VertexBindingDescription bindingDesc,
        PolygonMode polygonMode,
        PrimitiveTopology topology,
        CullMode cullMode,
        bool depthTestEnable,
        bool depthClamp,
        bool blendingEnable,
        Vector<DescriptorSet::Impl*>&& descriptorSets,
        PushConstant::Impl* pushConstant,
        uint32_t colorAttachmentCount,
        Vector<Format>&& colorFormats,
        Format depthFormat,
        SampleCount sampleCount
    )
    {
        auto layoutRes = CreatePipelineLayout(device, descriptorSets, pushConstant);
        if (!layoutRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create pipeline layout");
            return Unexpected(layoutRes.Error());
        }
        VkPipelineLayout pipelineLayout = layoutRes.Value();

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
        VkResult res = vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
            return Unexpected((VHResult)res);

        return Impl(device, pipeline, pipelineLayout, Pipeline::PipelineType::Graphics, VulkanHelper::Move(descriptorSets), pushConstant, UniquePtr<SBT>(nullptr));
    }

    Expected<Pipeline::Impl, VHResult> Pipeline::Impl::New(Device::Impl* device, Shader::Impl* computeShader, Vector<DescriptorSet::Impl*>&& descriptorSets, PushConstant::Impl* pushConstant)
    {
        // Validate that the shader is indeed a compute shader
        if (computeShader->GetShaderStage() != VK_SHADER_STAGE_COMPUTE_BIT)
        {
            VH_LOG_ERROR("Provided shader is not a compute shader!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        auto layoutRes = CreatePipelineLayout(device, descriptorSets, pushConstant);
        if (!layoutRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create pipeline layout");
            return Unexpected(layoutRes.Error());
        }
        VkPipelineLayout pipelineLayout = layoutRes.Value();

        // Create compute pipeline
        VkComputePipelineCreateInfo computePipelineInfo{};
        computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineInfo.stage = computeShader->GetShaderStageCreateInfo();
        computePipelineInfo.layout = pipelineLayout;
        computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        computePipelineInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        VkResult res = vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Couldn't create compute pipeline!");
            vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
            return Unexpected((VHResult)res);
        }

        return Impl(device, pipeline, pipelineLayout, Pipeline::PipelineType::Compute, VulkanHelper::Move(descriptorSets), pushConstant, UniquePtr<SBT>(nullptr));
    }

    Expected<Pipeline::Impl, VHResult> Pipeline::Impl::New(
        Device::Impl* device,
        Vector<Shader::Impl*>&& RayGenShaders,
        Vector<Shader::Impl*>&& HitShaders,
        Vector<Shader::Impl*>&& MissShaders,
        Vector<DescriptorSet::Impl*>&& descriptorSets,
        PushConstant::Impl* pushConstant,
        CommandBuffer::Impl* cmd
    )
    {
        VH_LOG_INFO("Creating RayTracing Pipeline Implementation");

        auto layoutRes = CreatePipelineLayout(device, descriptorSets, pushConstant);
        if (!layoutRes.HasValue())
        {
            VH_LOG_ERROR("Failed to create pipeline layout");
            return Unexpected(layoutRes.Error());
        }
        VkPipelineLayout pipelineLayout = layoutRes.Value();

        VulkanHelper::Vector<VkPipelineShaderStageCreateInfo> shaderStages;
        shaderStages.Reserve(RayGenShaders.Size() + HitShaders.Size() + MissShaders.Size());
        for (auto& shader : RayGenShaders)
            shaderStages.PushBack(shader->GetShaderStageCreateInfo());
        for (auto& shader : HitShaders)
            shaderStages.PushBack(shader->GetShaderStageCreateInfo());
        for (auto& shader : MissShaders)
            shaderStages.PushBack(shader->GetShaderStageCreateInfo());

        VulkanHelper::Vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
        shaderGroups.Reserve(RayGenShaders.Size() + HitShaders.Size() + MissShaders.Size());
        uint32_t stageIndex = 0;
        for (size_t i = 0; i < RayGenShaders.Size(); i++)
        {
            VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
            groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            groupInfo.generalShader = stageIndex;
            groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
            groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
            groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.PushBack(groupInfo);

            stageIndex++;
        }

        for (size_t i = 0; i < HitShaders.Size(); i++)
        {
            VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
            groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
            groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
            groupInfo.closestHitShader = stageIndex;
            groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
            groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.PushBack(groupInfo);

            stageIndex++;
        }

        for (size_t i = 0; i < MissShaders.Size(); i++)
        {
            VkRayTracingShaderGroupCreateInfoKHR groupInfo{};
            groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
            groupInfo.generalShader = stageIndex;
            groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
            groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
            groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
            shaderGroups.PushBack(groupInfo);

            stageIndex++;
        }

        VkRayTracingPipelineCreateInfoKHR rayTracingPipelineInfo{};
        rayTracingPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        rayTracingPipelineInfo.stageCount = (uint32_t)shaderStages.Size();
        rayTracingPipelineInfo.pStages = shaderStages.Data();
        rayTracingPipelineInfo.groupCount = (uint32_t)shaderGroups.Size();
        rayTracingPipelineInfo.pGroups = shaderGroups.Data();
        rayTracingPipelineInfo.maxPipelineRayRecursionDepth = 1; // Don't support recursion, it's way slower than loop based ray tracing
        rayTracingPipelineInfo.layout = pipelineLayout;
        rayTracingPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        rayTracingPipelineInfo.basePipelineIndex = -1;

        VkPipeline pipeline;
        VkResult res = FunctionLoader::vkCreateRayTracingPipelinesKHR(device->GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineInfo, nullptr, &pipeline);
        if (res != VK_SUCCESS)
        {
            VH_LOG_ERROR("Couldn't create ray tracing pipeline!");
            vkDestroyPipelineLayout(device->GetDevice(), pipelineLayout, nullptr);
            return Unexpected((VHResult)res);
        }

        auto sbtRes = SBT::New(device, pipeline, (uint32_t)RayGenShaders.Size(), (uint32_t)MissShaders.Size(), (uint32_t)HitShaders.Size(), cmd);
        if (!sbtRes)
        {
            VH_LOG_ERROR("Couldn't create SBT!");
            vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
            return Unexpected(sbtRes.Error());
        }
        UniquePtr<SBT> sbt(new SBT(Move(sbtRes.Value())));

        return Impl(device, pipeline, pipelineLayout, Pipeline::PipelineType::RayTracing, VulkanHelper::Move(descriptorSets), pushConstant, Move(sbt));
    }

    Pipeline::Impl::~Impl()
    {
        if (m_Pipeline != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Queuing Pipeline Implementation for deletion");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Pipeline);
            m_Device->GetDeleteQueue().QueueForDeletion(m_Layout);
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
        , m_DescriptorSets(Move(other.m_DescriptorSets))
        , m_PushConstant(other.m_PushConstant)
        , m_SBT(Move(other.m_SBT))
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
        m_DescriptorSets = Move(other.m_DescriptorSets);
        m_SBT = Move(other.m_SBT);

        return *this;
    }

    void Pipeline::Impl::Bind(CommandBuffer::Impl* commandBuffer)
    {
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
        vkCmdBindPipeline(commandBuffer->GetCommandBuffer(), bindPoint, m_Pipeline);

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
                commandBuffer->GetCommandBuffer(),
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
                commandBuffer->GetCommandBuffer(),
                m_Layout,
                range.stageFlags,
                range.offset,
                range.size,
                m_PushConstant->GetData()
            );
        }
    }

    void Pipeline::Impl::Dispatch(CommandBuffer::Impl* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        VH_ASSERT(m_PipelineType == Pipeline::PipelineType::Compute, "Dispatch can only be called on compute pipelines!");

        vkCmdDispatch(commandBuffer->GetCommandBuffer(), groupCountX, groupCountY, groupCountZ);
    }

    Expected<VkPipelineLayout, VHResult> Pipeline::Impl::CreatePipelineLayout(Device::Impl* device, const Vector<DescriptorSet::Impl*>& descriptorSets, PushConstant::Impl* pushConstant)
    {
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

        return pipelineLayout;
    }

    void Pipeline::Impl::RayTrace(CommandBuffer::Impl* commandBuffer, uint32_t width, uint32_t height, uint32_t depth)
    {
        VH_ASSERT(m_PipelineType == Pipeline::PipelineType::RayTracing, "RayTrace can only be called on ray tracing pipelines!");

        FunctionLoader::vkCmdTraceRaysKHR(
            commandBuffer->GetCommandBuffer(),
            m_SBT->GetRgenRegionPtr(),
            m_SBT->GetMissRegionPtr(),
            m_SBT->GetHitRegionPtr(),
            m_SBT->GetHitRegionPtr(), // Pass hit region as callable, callable shaders are not supported yet but vulkan needs something to point to. So PLEASE don't use callable shaders
            width,
            height,
            depth
        );
    }

    //
    // Forward functions
    //
    
    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const GraphicsConfig& config)
    {
        VH_LOG_INFO("Creating Graphics Pipeline Implementation");

        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Device Cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<Shader::Impl*> shaders;
        shaders.Reserve(config.Shaders.Size());
        for (auto& shader : config.Shaders)
        {
            if (shader == nullptr)
            {
                VH_LOG_ERROR("Shader cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            shaders.PushBack(Shader::Impl::GetImplementation(shader));
        }

        VulkanHelper::Vector<DescriptorSet::Impl*> descriptorSets;
        descriptorSets.Reserve(config.DescriptorSets.Size());
        for (auto& descriptorSet : config.DescriptorSets)
        {
            if (descriptorSet == nullptr)
            {
                VH_LOG_ERROR("Descriptor Set cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            descriptorSets.PushBack(DescriptorSet::Impl::GetImplementation(descriptorSet));
        }

        PushConstant::Impl* pushConstant = nullptr;
        if (config.PushConstant != nullptr)
            pushConstant = PushConstant::Impl::GetImplementation(config.PushConstant);

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            Move(shaders),
            config.AttributeDesc,
            config.BindingDesc,
            config.PolygonMode,
            config.Topology,
            config.CullMode,
            config.DepthTestEnable,
            config.DepthClamp,
            config.BlendingEnable,
            Move(descriptorSets),
            pushConstant,
            config.ColorAttachmentCount,
            Move(config.ColorFormats.Clone()),
            config.DepthFormat,
            config.SampleCount
        );

        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Pipeline::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const ComputeConfig& config)
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

        VulkanHelper::Vector<DescriptorSet::Impl*> descriptorSets;
        descriptorSets.Reserve(config.DescriptorSets.Size());
        for (auto& descriptorSet : config.DescriptorSets)
        {
            if (descriptorSet == nullptr)
            {
                VH_LOG_ERROR("Descriptor Set cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            descriptorSets.PushBack(DescriptorSet::Impl::GetImplementation(descriptorSet));
        }

        PushConstant::Impl* pushConstant = nullptr;
        if (config.PushConstant != nullptr)
            pushConstant = PushConstant::Impl::GetImplementation(config.PushConstant);

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            Shader::Impl::GetImplementation(config.ComputeShader), 
            Move(descriptorSets),
            pushConstant
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Pipeline::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    VulkanHelper::Expected<Pipeline, VHResult> Pipeline::New(const RayTracingConfig& config)
    {
        VH_LOG_INFO("Creating Compute Pipeline Implementation");

        if (config.Device == nullptr)
        {
            VH_LOG_ERROR("Device cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (config.CommandBuffer == nullptr)
        {
            VH_LOG_ERROR("CommandBuffer cannot be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<Shader::Impl*> rayGenShaders;
        rayGenShaders.Reserve(config.RayGenShaders.Size());
        for (auto& shader : config.RayGenShaders)
        {
            if (shader == nullptr)
            {
                VH_LOG_ERROR("Ray Generation shader cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            rayGenShaders.PushBack(Shader::Impl::GetImplementation(shader));
        }

        VulkanHelper::Vector<Shader::Impl*> hitShaders;
        hitShaders.Reserve(config.HitShaders.Size());
        for (auto& shader : config.HitShaders)
        {
            if (shader == nullptr)
            {
                VH_LOG_ERROR("Hit shader cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            hitShaders.PushBack(Shader::Impl::GetImplementation(shader));
        }

        VulkanHelper::Vector<Shader::Impl*> missShaders;
        missShaders.Reserve(config.MissShaders.Size());
        for (auto& shader : config.MissShaders)
        {
            if (shader == nullptr)
            {
                VH_LOG_ERROR("Miss shader cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            missShaders.PushBack(Shader::Impl::GetImplementation(shader));
        }
        
        PushConstant::Impl* pushConstant = nullptr;
        if (config.PushConstant != nullptr)
            pushConstant = PushConstant::Impl::GetImplementation(config.PushConstant);

        VulkanHelper::Vector<DescriptorSet::Impl*> descriptorSets;
        descriptorSets.Reserve(config.DescriptorSets.Size());
        for (auto& descriptorSet : config.DescriptorSets)
        {
            if (descriptorSet == nullptr)
            {
                VH_LOG_ERROR("Descriptor Set cannot be nullptr!");
                return Unexpected(VHResult::WRONG_ARGUMENTS);
            }
            descriptorSets.PushBack(DescriptorSet::Impl::GetImplementation(descriptorSet));
        }

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            Move(rayGenShaders),
            Move(hitShaders),
            Move(missShaders),
            Move(descriptorSets),
            pushConstant,
            CommandBuffer::Impl::GetImplementation(config.CommandBuffer)
        );
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Pipeline::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
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

    void Pipeline::Bind(CommandBuffer& commandBuffer)
    {
        m_Impl->Bind(CommandBuffer::Impl::GetImplementation(&commandBuffer));
    }

    void Pipeline::Dispatch(CommandBuffer& commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        m_Impl->Dispatch(CommandBuffer::Impl::GetImplementation(&commandBuffer), groupCountX, groupCountY, groupCountZ);
    }

    void Pipeline::RayTrace(CommandBuffer& commandBuffer, uint32_t width, uint32_t height, uint32_t depth)
    {
        m_Impl->RayTrace(CommandBuffer::Impl::GetImplementation(&commandBuffer), width, height, depth);
    }
}