#include "pch.h"
#include "Tonemap.h"

#include "Vulkan/Swapchain.h"
#include "Renderer/Renderer.h"
#include "Vulkan/DeleteQueue.h"

#include "Core/Window.h"

namespace VulkanHelper
{

	void Tonemap::Init(const CreateInfo& info)
	{
		if (m_Initialized)
			Destroy();

		m_ImageSize = info.OutputImage->GetImageSize();
		m_Context = info.Context;

		m_InputImage = info.InputImage;
		m_OutputImage = info.OutputImage;

		{
			VulkanHelper::DescriptorSetLayout::Binding bin{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT };
			VulkanHelper::DescriptorSetLayout::Binding bin1{ 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT };

			m_Descriptor.Init(&m_Context.Window->GetRenderer()->GetDescriptorPool(), {bin, bin1});
			m_Descriptor.AddImageSampler(
				0,
				{ m_Context.Window->GetRenderer()->GetLinearSampler().GetSamplerHandle(),
				info.InputImage->GetImageView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
			);
			m_Descriptor.AddImageSampler(
				1,
				{ m_Context.Window->GetRenderer()->GetLinearSampler().GetSamplerHandle(),
				info.OutputImage->GetImageView(),
				VK_IMAGE_LAYOUT_GENERAL }
			);
			m_Descriptor.Build();
		}

		// Pipeline
		{
			m_Push.Init({ VK_SHADER_STAGE_COMPUTE_BIT });

			DescriptorSetLayout::Binding bin{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout::Binding bin1{ 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout imageLayout({ bin, bin1 });

			std::string currentTonemapper = GetTonemapperMacroDefinition(m_CurrentTonemapper);

			Pipeline::ComputeCreateInfo pipelineInfo{};

			std::vector<Shader::Define> defines = { {currentTonemapper, ""} };
			if (m_EnableChromaticAberration)
				defines.emplace_back(Shader::Define{ "USE_CHROMATIC_ABERRATION", "" });

			Shader shader({ "../VulkanHelper/src/VulkanHelper/Shaders/Tonemap.glsl" , VK_SHADER_STAGE_COMPUTE_BIT, defines, true });
			pipelineInfo.Shader = &shader;

			pipelineInfo.DescriptorSetLayouts = {
				imageLayout.GetDescriptorSetLayoutHandle()
			};;
			pipelineInfo.PushConstants = m_Push.GetRangePtr();
			pipelineInfo.debugName = "Tone Map Pipeline";

			m_Pipeline.Init(pipelineInfo);
		}

		m_Initialized = true;
	}

	void Tonemap::Destroy()
	{
		if (!m_Initialized)
			return;

		m_Pipeline.Destroy();
		
		m_Descriptor.Destroy();
		m_Push.Destroy();

		Reset();
	}

	void Tonemap::RecompileShader(Tonemappers tonemapper, bool chromaticAberration)
	{
		// Pipeline
		{
			DescriptorSetLayout::Binding bin{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout::Binding bin1{ 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout imageLayout({ bin, bin1 });

			std::string currentTonemapper = GetTonemapperMacroDefinition(tonemapper);
			std::vector<Shader::Define> defines = { {currentTonemapper, ""} };
			if (chromaticAberration)
				defines.emplace_back(Shader::Define{ "USE_CHROMATIC_ABERRATION", ""});

			Pipeline::ComputeCreateInfo info{};
			Shader shader({ "../VulkanHelper/src/VulkanHelper/Shaders/Tonemap.glsl", VK_SHADER_STAGE_COMPUTE_BIT, defines, true });
			info.Shader = &shader;

			info.DescriptorSetLayouts = {
				imageLayout.GetDescriptorSetLayoutHandle()
			};;
			info.PushConstants = m_Push.GetRangePtr();
			info.debugName = "Tone Map Pipeline";

			m_Pipeline.Init(info);
		}
	}

	Tonemap::Tonemap(const CreateInfo& info)
	{
		Init(info);
	}

	Tonemap::Tonemap(Tonemap&& other) noexcept
	{
		if (m_Initialized)
			Destroy();

		m_Descriptor = std::move(other.m_Descriptor);
		m_Pipeline = std::move(other.m_Pipeline);
		m_Push = std::move(other.m_Push);
		m_ImageSize	= std::move(other.m_ImageSize);
		m_InputImage = std::move(other.m_InputImage);
		m_OutputImage = std::move(other.m_OutputImage);
		m_Initialized = std::move(other.m_Initialized);

		other.Reset();
	}

	Tonemap& Tonemap::operator=(Tonemap&& other) noexcept
	{
		if (m_Initialized)
			Destroy();

		m_Descriptor = std::move(other.m_Descriptor);
		m_Pipeline = std::move(other.m_Pipeline);
		m_Push = std::move(other.m_Push);
		m_ImageSize = std::move(other.m_ImageSize);
		m_InputImage = std::move(other.m_InputImage);
		m_OutputImage = std::move(other.m_OutputImage);
		m_Initialized = std::move(other.m_Initialized);

		other.Reset();

		return *this;
	}

	Tonemap::~Tonemap()
	{
		Destroy();
	}

	void Tonemap::Run(const TonemapInfo& info, VkCommandBuffer cmd)
	{
		if (m_CurrentTonemapper != info.Tonemapper || m_EnableChromaticAberration != info.ChromaticAberration)
		{
			m_CurrentTonemapper = info.Tonemapper;
			m_EnableChromaticAberration = info.ChromaticAberration;
			RecompileShader(m_CurrentTonemapper, m_EnableChromaticAberration);
		}

		m_OutputImage->TransitionImageLayout(
			VK_IMAGE_LAYOUT_GENERAL,
			m_Context.Window->GetRenderer()->GetCurrentCommandBuffer()
		);

		m_InputImage->TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_Context.Window->GetRenderer()->GetCurrentCommandBuffer());

		m_Pipeline.Bind(cmd);

		m_Descriptor.Bind(
			0,
			m_Pipeline.GetPipelineLayout(),
			VK_PIPELINE_BIND_POINT_COMPUTE, 
			cmd
		);

		*(m_Push.GetDataPtr()) = info;

		m_Push.Push(m_Pipeline.GetPipelineLayout(), cmd);

		vkCmdDispatch(cmd, ((int)m_ImageSize.width) / 8 + 1, ((int)m_ImageSize.height) / 8 + 1, 1);
	}

	std::string Tonemap::GetTonemapperMacroDefinition(Tonemappers tonemapper)
	{
		switch (tonemapper)
		{
		case VulkanHelper::Tonemap::Filmic:
			return "USE_FILMIC";
			break;
		case VulkanHelper::Tonemap::HillAces:
			return "USE_ACES_HILL";
			break;
		case VulkanHelper::Tonemap::NarkowiczAces:
			return "USE_ACES_NARKOWICZ";
			break;
		case VulkanHelper::Tonemap::ExposureMapping:
			return "USE_EXPOSURE_MAPPING";
			break;
		case VulkanHelper::Tonemap::Uncharted2:
			return "USE_UNCHARTED";
			break;
		case VulkanHelper::Tonemap::ReinchardExtended:
			return "USE_REINHARD_EXTENDED";
			break;
		default:
			return "USE_FILMIC";
			break;
		}
	}

	void Tonemap::Reset()
	{
		m_ImageSize = { 0, 0 };
		m_InputImage = nullptr;
		m_OutputImage = nullptr;
		m_Initialized = false;

		m_CurrentTonemapper = Tonemappers::Filmic;
		m_EnableChromaticAberration = false;
	}

}