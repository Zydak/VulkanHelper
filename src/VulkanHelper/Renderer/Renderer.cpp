#include "pch.h"
#include "Renderer.h"
#include "Scene/Scene.h"
#include "Scene/Components.h"
#include "Core/Window.h"
#include "Vulkan/Instance.h"

#include "lodepng.h"

#ifdef VL_IMGUI
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

void ImGui_ImplVulkan_DestroyDeviceObjects();
#endif

namespace VulkanHelper
{
	void Renderer::Destroy()
	{
		if (!m_Initialized)
			return;

		m_Initialized = false;
		vkDeviceWaitIdle(Device::GetDevice());
		vkFreeCommandBuffers(Device::GetDevice(), Device::GetGraphicsCommandPool(), (uint32_t)m_CommandBuffers.size(), m_CommandBuffers.data());

		m_Swapchain.reset();

		m_EnvToCubemapDescriptorSet.reset();

		m_RendererLinearSampler.Destroy();
		m_RendererLinearSamplerRepeat.Destroy();
		m_RendererNearestSampler.Destroy();

		m_HDRToPresentablePipeline.Destroy();
		m_EnvToCubemapPipeline.Destroy();

		m_QuadMesh.Destroy();
		m_Pool.reset();

#ifdef VL_IMGUI
		DestroyImGui();
#endif
	}

	static void CheckVkResult(VkResult err)
	{
		if (err == 0) return;
		fprintf(stderr, "[Vulkan] Error: VkResult = %d\n", err);
		if (err < 0) abort();
	}

	Renderer::Renderer(Renderer&& other) noexcept
	{
		m_MaxFramesInFlight = std::move(other.m_MaxFramesInFlight);
		m_Pool = std::move(other.m_Pool);
		m_Context = std::move(other.m_Context);
		m_CommandBuffers = std::move(other.m_CommandBuffers);
		m_Swapchain = std::move(other.m_Swapchain);
		m_IsFrameStarted = std::move(other.m_IsFrameStarted);
		m_CurrentImageIndex = std::move(other.m_CurrentImageIndex);
		m_CurrentFrameIndex = std::move(other.m_CurrentFrameIndex);
		m_QuadMesh = std::move(other.m_QuadMesh);
		m_RendererLinearSampler = std::move(other.m_RendererLinearSampler);
		m_RendererLinearSamplerRepeat = std::move(other.m_RendererLinearSamplerRepeat);
		m_RendererNearestSampler = std::move(other.m_RendererNearestSampler);
		m_EnvToCubemapDescriptorSet = std::move(other.m_EnvToCubemapDescriptorSet);
		m_HDRToPresentablePipeline = std::move(other.m_HDRToPresentablePipeline);
		m_EnvToCubemapPipeline = std::move(other.m_EnvToCubemapPipeline);
		m_ImGuiFunction = std::move(other.m_ImGuiFunction);
		m_Initialized = std::move(other.m_Initialized);
	}

	Renderer& Renderer::operator=(Renderer&& other) noexcept
	{
		m_MaxFramesInFlight = std::move(other.m_MaxFramesInFlight);
		m_Pool = std::move(other.m_Pool);
		m_Context = std::move(other.m_Context);
		m_CommandBuffers = std::move(other.m_CommandBuffers);
		m_Swapchain = std::move(other.m_Swapchain);
		m_IsFrameStarted = std::move(other.m_IsFrameStarted);
		m_CurrentImageIndex = std::move(other.m_CurrentImageIndex);
		m_CurrentFrameIndex = std::move(other.m_CurrentFrameIndex);
		m_QuadMesh = std::move(other.m_QuadMesh);
		m_RendererLinearSampler = std::move(other.m_RendererLinearSampler);
		m_RendererLinearSamplerRepeat = std::move(other.m_RendererLinearSamplerRepeat);
		m_RendererNearestSampler = std::move(other.m_RendererNearestSampler);
		m_EnvToCubemapDescriptorSet = std::move(other.m_EnvToCubemapDescriptorSet);
		m_HDRToPresentablePipeline = std::move(other.m_HDRToPresentablePipeline);
		m_EnvToCubemapPipeline = std::move(other.m_EnvToCubemapPipeline);
		m_ImGuiFunction = std::move(other.m_ImGuiFunction);
		m_Initialized = std::move(other.m_Initialized);

		return *this;
	}

	void Renderer::Init(const VulkanHelperContext& context, uint32_t maxFramesInFlight)
	{
		m_MaxFramesInFlight = maxFramesInFlight;

		CreatePool();
		m_RendererLinearSampler.Init(SamplerInfo(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR));
		m_RendererLinearSamplerRepeat.Init(SamplerInfo(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR));
		m_RendererNearestSampler.Init(SamplerInfo(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST));

		m_Initialized = true;
		m_Context = context;
		CreateDescriptorSets();
		RecreateSwapchain();
		CreateCommandBuffers();

		// Vertices for a simple quad
		const std::vector<Mesh::Vertex> vertices = //-V826
		{
			Mesh::Vertex(glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f)),  // Vertex 1 Bottom Left
			Mesh::Vertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)), // Vertex 2 Top Left
			Mesh::Vertex(glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)),  // Vertex 3 Top Right
			Mesh::Vertex(glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f))    // Vertex 4 Bottom Right
		};

		const std::vector<uint32_t> indices = //-V826
		{
			0, 1, 2,  // First triangle
			0, 2, 3   // Second triangle
		};


		m_QuadMesh.Init({ &vertices, &indices });

		InitImGui();
	}

	void Renderer::SetImGuiFunction(std::function<void()> fn)
	{
		m_ImGuiFunction = fn;
	}

	void Renderer::RayTrace(VkCommandBuffer cmdBuf, SBT* sbt, VkExtent2D imageSize, uint32_t depth /* = 1*/)
	{
		Device::vkCmdTraceRaysKHR(
			cmdBuf,
			sbt->GetRGenRegionPtr(),
			sbt->GetMissRegionPtr(),
			sbt->GetHitRegionPtr(),
			sbt->GetCallRegionPtr(),
			imageSize.width,
			imageSize.height,
			1
		);
	}

	/*
	 * @brief Acquires the next swap chain image and begins recording a command buffer for rendering.
	 * If the swap chain is out of date, it may trigger swap chain recreation.
	 *
	 * @return True if the frame started successfully; false if the swap chain needs recreation.
	 */
	bool Renderer::BeginFrame()
	{
		VK_CORE_ASSERT(!m_IsFrameStarted, "Can't call BeginFrame while already in progress!");

		auto extent = m_Context.Window->GetExtent();
		if (extent.width == 0 || extent.height == 0)
		{
			// If the extent is 0 the window is either minimized or someone resized it to 0. In either case skip the rendering
			return false;
		}

		if (m_Context.Window->WasWindowResized())
		{
			m_Context.Window->ResetWindowResizedFlag();
			RecreateSwapchain();

			//return false;
		}

		auto result = m_Swapchain->AcquireNextImage(m_CurrentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapchain();

			//return false;
		}
		VK_CORE_ASSERT(result == VK_SUCCESS, "failed to acquire swap chain image!");

		m_IsFrameStarted = true;
		auto commandBuffer = GetCurrentCommandBuffer();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CORE_RETURN_ASSERT(
			vkBeginCommandBuffer(commandBuffer, &beginInfo),
			VK_SUCCESS,
			"failed to begin recording command buffer!"
		);
		return true;
	}

	/*
	 * @brief Finalizes the recorded command buffer, submits it for execution, and presents the swap chain image.
	 */
	void Renderer::EndFrame() 
	{
		bool retVal = true;
		auto commandBuffer = GetCurrentCommandBuffer();
		VK_CORE_ASSERT(m_IsFrameStarted, "Cannot call EndFrame while frame is not in progress");

		// End recording the command buffer
		auto success = vkEndCommandBuffer(commandBuffer);
		VK_CORE_ASSERT(success == VK_SUCCESS, "Failed to record command buffer!");

		m_Swapchain->SubmitCommandBuffers(&commandBuffer, m_CurrentImageIndex);

		// End the frame and update frame index
		m_IsFrameStarted = false;
		m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;
	}

	/*
	 * @brief Sets up the rendering viewport, scissor, and begins the specified render pass on the given framebuffer.
	 * It also clears the specified colors in the render pass.
	 *
	 * @param clearColors - A vector of clear values for the attachments in the render pass.
	 * @param framebuffer - The framebuffer to use in the render pass.
	 * @param renderPass - The render pass to begin.
	 * @param extent - The extent (width and height) of the render area.
	 */
	void Renderer::BeginRenderPass(const std::vector<VkClearValue>& clearColors, VkFramebuffer framebuffer, VkRenderPass renderPass, VkExtent2D extent)
	{
		VK_CORE_ASSERT(m_IsFrameStarted, "Cannot call BeginSwapchainRenderPass while frame is not in progress");

		// Set up viewport
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)extent.width;
		viewport.height = (float)extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// Set up scissor
		VkRect2D scissor{
			{0, 0},
			VkExtent2D{(uint32_t)extent.width, (uint32_t)extent.height}
		};

		// Set viewport and scissor for the current command buffer
		vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);
		vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);

		// Set up render pass information
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = VkExtent2D{ (uint32_t)extent.width, (uint32_t)extent.height };
		renderPassInfo.clearValueCount = (uint32_t)clearColors.size();
		renderPassInfo.pClearValues = clearColors.data();

		// Begin the render pass for the current command buffer
		vkCmdBeginRenderPass(GetCurrentCommandBuffer(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	/**
	 * @brief Ends the current render pass in progress.
	 * It should be called after rendering commands within a render pass have been recorded.
	 */
	void Renderer::EndRenderPass()
	{
		VK_CORE_ASSERT(m_IsFrameStarted, "Can't call EndSwapchainRenderPass while frame is not in progress");
		
		vkCmdEndRenderPass(GetCurrentCommandBuffer());
	}

	// TODO: description
	void Renderer::SaveImageToFile(const std::string& filepath, Image* image)
	{
		VkCommandBuffer cmd;
		Device::BeginSingleTimeCommands(cmd, Device::GetGraphicsCommandPool());

		image->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);

		VkExtent2D s = image->GetImageSize();
		int width = s.width;
		int height = s.height;

		Image::CreateInfo imageInfo{};
		imageInfo.Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
		imageInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		imageInfo.Height = height;
		imageInfo.Width = width;
		imageInfo.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		imageInfo.SamplerInfo = SamplerInfo{};
		imageInfo.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		Image image8Bit(imageInfo);
		image8Bit.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd);

		VkImageBlit blitRegion{};
		blitRegion.dstOffsets[0] = { 0, 0, 0 };
		blitRegion.dstOffsets[1] = { width, height, 1 };
		blitRegion.srcOffsets[0] = { 0, 0, 0 };
		blitRegion.srcOffsets[1] = { width, height, 1 };
		blitRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		blitRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

		vkCmdBlitImage(cmd, image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image8Bit.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_LINEAR);
		image8Bit.TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);

		Buffer::CreateInfo info{};
		info.InstanceSize = width * height * 4;
		info.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		info.UsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		Buffer tempBuffer(info);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.imageExtent = { image->GetImageSize().width, image->GetImageSize().height, 1 };
		region.imageOffset = { 0, 0, 0 };
		region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		tempBuffer.Map();

		vkCmdCopyImageToBuffer(cmd, image8Bit.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tempBuffer.GetBuffer(), 1, &region);
		Device::EndSingleTimeCommands(cmd, Device::GetGraphicsQueue(), Device::GetGraphicsCommandPool());

		vkDeviceWaitIdle(Device::GetDevice());

		void* bufferData = tempBuffer.GetMappedMemory();

		std::vector<unsigned char> imageBuffer(width * height * 4);
		memcpy(imageBuffer.data(), bufferData, width * height * 4);

		if (filepath.empty())
		{
			std::string path = "Rendered_Images/";
			if (!std::filesystem::exists(path))
				std::filesystem::create_directory(path);

			uint32_t ID = 0;
			
			while (std::filesystem::exists("Rendered_Images/Render" + std::to_string(ID) + ".png"))
			{
				ID++;
			}

			WriteToFile(std::string("Rendered_Images/Render" + std::to_string(ID) + ".png").c_str(), imageBuffer, width, height);
		}
		else
		{
			WriteToFile(filepath.c_str(), imageBuffer, width, height);
		}
	}

	// TODO: delete this?
	/**
	 * @brief Takes descriptor set with single combined image sampler descriptor and copies data
	 * from the image onto presentable swapchain framebuffer
	 *
	 * @param descriptorWithImageSampler - descriptor set with single image sampler
	 */
	void Renderer::FramebufferCopyPassImGui(DescriptorSet* descriptorWithImageSampler)
	{
		std::vector<VkClearValue> clearColors;
		clearColors.emplace_back(VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f });
		// Begin the render pass
		BeginRenderPass(
			clearColors,
			m_Swapchain->GetPresentableFrameBuffer(m_CurrentImageIndex),
			m_Swapchain->GetSwapchainRenderPass(),
			m_Swapchain->GetSwapchainExtent()
		);
		// Bind the geometry pipeline
		m_HDRToPresentablePipeline.Bind(GetCurrentCommandBuffer());

		descriptorWithImageSampler->Bind(
			0,
			m_HDRToPresentablePipeline.GetPipelineLayout(),
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			GetCurrentCommandBuffer()
		);

		m_QuadMesh.Bind(GetCurrentCommandBuffer());
		m_QuadMesh.Draw(GetCurrentCommandBuffer(), 1);

#ifdef VL_IMGUI
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		m_ImGuiFunction();

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VulkanHelper::Renderer::GetCurrentCommandBuffer());
#endif
		// End the render pass
		EndRenderPass();
	}

	void Renderer::ImGuiPass()
	{
#ifdef VL_IMGUI
		std::vector<VkClearValue> clearColors;
		clearColors.emplace_back(VkClearValue{ 0.0f, 0.0f, 0.0f, 0.0f });
		BeginRenderPass(
			clearColors,
			m_Swapchain->GetPresentableFrameBuffer(m_CurrentImageIndex),
			m_Swapchain->GetSwapchainRenderPass(),
			m_Swapchain->GetSwapchainExtent()
		);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		m_ImGuiFunction();

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), VulkanHelper::Renderer::GetCurrentCommandBuffer());
		EndRenderPass();
#endif
	}

	// TODO description, note that image has to be in transfer src optimal layout
	void Renderer::FramebufferCopyPassBlit(Image* image)
	{
		image->TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, GetCurrentCommandBuffer());

		Image::TransitionImageLayout(
			m_Swapchain->GetPresentableImage(m_CurrentImageIndex),
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			GetCurrentCommandBuffer()
		);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { (int32_t)image->GetImageSize().width, (int32_t)image->GetImageSize().height, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { (int32_t)m_Swapchain->GetSwapchainExtent().width, (int32_t)m_Swapchain->GetSwapchainExtent().height, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(
			GetCurrentCommandBuffer(),
			image->GetImage(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_Swapchain->GetPresentableImage(m_CurrentImageIndex),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_NEAREST
		);

		Image::TransitionImageLayout(
			m_Swapchain->GetPresentableImage(m_CurrentImageIndex),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			0,
			GetCurrentCommandBuffer()
		);
	}

	void Renderer::EnvMapToCubemapPass(Ref<Image> envMap, Ref<Image> cubemap)
	{
		{
			DescriptorSetLayout::Binding bin{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout::Binding bin1{ 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT };
			m_EnvToCubemapDescriptorSet = std::make_shared<VulkanHelper::DescriptorSet>();
			m_EnvToCubemapDescriptorSet->Init(&VulkanHelper::Renderer::GetDescriptorPool(), { bin, bin1 });
			m_EnvToCubemapDescriptorSet->AddImageSampler(0, { GetLinearSampler().GetSamplerHandle(), envMap->GetImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
			m_EnvToCubemapDescriptorSet->AddImageSampler(1, { GetLinearSampler().GetSamplerHandle(), cubemap->GetImageView(), VK_IMAGE_LAYOUT_GENERAL });
			m_EnvToCubemapDescriptorSet->Build();
		}

		VkCommandBuffer cmdBuf;
		Device::BeginSingleTimeCommands(cmdBuf, Device::GetGraphicsCommandPool());

		cubemap->TransitionImageLayout(VK_IMAGE_LAYOUT_GENERAL, cmdBuf);

		m_EnvToCubemapPipeline.Bind(cmdBuf);
		m_EnvToCubemapDescriptorSet->Bind(0, m_EnvToCubemapPipeline.GetPipelineLayout(), VK_PIPELINE_BIND_POINT_COMPUTE, cmdBuf);

		vkCmdDispatch(cmdBuf, cubemap->GetImageSize().width / 8 + 1, cubemap->GetImageSize().height / 8 + 1, 1);

		cubemap->TransitionImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdBuf);

		Device::EndSingleTimeCommands(cmdBuf, Device::GetGraphicsQueue(), Device::GetGraphicsCommandPool());
	}

	/*
	 * @brief This function is called when the window is resized or the swapchain needs to be recreated for any reason.
	 */
	void Renderer::RecreateSwapchain()
	{
		auto extent = m_Context.Window->GetExtent();

		// Wait for the device to be idle before recreating the swapchain
		vkDeviceWaitIdle(Device::GetDevice());

		// Recreate the swapchain
		if (m_Swapchain == nullptr)
		{
			m_Swapchain = std::make_unique<Swapchain>(Swapchain::CreateInfo{ extent, PresentModes::VSync, m_MaxFramesInFlight, nullptr, m_Context.Window->GetSurface() });
		}
		else
		{
			// Move the old swapchain into a shared pointer to ensure it is properly cleaned up
			std::shared_ptr<Swapchain> oldSwapchain = std::move(m_Swapchain);

			// Create a new swapchain using the old one as a reference
			m_Swapchain = std::make_unique<Swapchain>(Swapchain::CreateInfo{ extent, PresentModes::VSync, m_MaxFramesInFlight, oldSwapchain, m_Context.Window->GetSurface() });

			// Check if the swap formats are consistent
			VK_CORE_ASSERT(oldSwapchain->CompareSwapFormats(*m_Swapchain), "Swap chain image or depth formats have changed!");
		}

		// Recreate other resources dependent on the swapchain, such as pipelines or framebuffers
		CreatePipeline();
	}

	/*
	 * @brief Allocates primary command buffers from the command pool for each swap chain image.
	 */
	void Renderer::CreateCommandBuffers()
	{
		// Resize the command buffer array to match the number of swap chain images
		m_CommandBuffers.resize(m_Swapchain->GetImageCount());

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = Device::GetGraphicsCommandPool();
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		// Allocate primary command buffers
		VK_CORE_RETURN_ASSERT(
			vkAllocateCommandBuffers(Device::GetDevice(), &allocInfo, m_CommandBuffers.data()),
			VK_SUCCESS,
			"Failed to allocate command buffers!"
		);

		for (uint32_t i = 0; i < m_Swapchain->GetImageCount(); i++)
		{
			Device::SetObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)m_CommandBuffers[i], "Main Frame Command Buffer");
		}
	}

	/*
	 * @brief Creates the descriptor pool for managing descriptor sets.
	 */
	void Renderer::CreatePool()
	{
		// Create and initialize the descriptor pool
		std::vector<DescriptorPool::PoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, (m_MaxFramesInFlight) * 100 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (m_MaxFramesInFlight) * 10000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, (m_MaxFramesInFlight) * 100 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, (m_MaxFramesInFlight) * 100 },
		};

		if (Device::UseRayTracing())
			poolSizes.emplace_back(DescriptorPool::PoolSize{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, (m_MaxFramesInFlight) * 100 });
		m_Pool = std::make_unique<DescriptorPool>(poolSizes, (m_MaxFramesInFlight) * 10000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	}

	/**
	 * @brief Creates the graphics pipeline for rendering geometry.
	 */
	void Renderer::CreatePipeline()
	{
		//
		// HDR to presentable
		//

		{
			DescriptorSetLayout::Binding bin{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT };
			DescriptorSetLayout imageLayout({ bin });

			Pipeline::GraphicsCreateInfo info{};
			info.AttributeDesc = Mesh::Vertex::GetAttributeDescriptions();
			info.BindingDesc = Mesh::Vertex::GetBindingDescriptions();

			VulkanHelper::Shader shader1({ "../VulkanHelper/src/VulkanHelper/Shaders/HDRToPresentableVert.glsl", VK_SHADER_STAGE_VERTEX_BIT, {}, true });
			VulkanHelper::Shader shader2({ "../VulkanHelper/src/VulkanHelper/Shaders/HDRToPresentableFrag.glsl", VK_SHADER_STAGE_FRAGMENT_BIT, {}, true });
			info.Shaders.push_back(&shader1);
			info.Shaders.push_back(&shader2);
			info.CullMode = VK_CULL_MODE_BACK_BIT;
			info.Width = m_Swapchain->GetWidth();
			info.Height = m_Swapchain->GetHeight();
			info.RenderPass = m_Swapchain->GetSwapchainRenderPass();

			info.DescriptorSetLayouts = {
				imageLayout.GetDescriptorSetLayoutHandle()
			};

			info.debugName = "HDR To Presentable Pipeline";

			// Create the graphics pipeline
			m_HDRToPresentablePipeline.Init(info);
		}

		// Env to cubemap
		{
			DescriptorSetLayout::Binding bin{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout::Binding bin1{ 1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT };
			DescriptorSetLayout imageLayout({ bin, bin1 });

			Pipeline::ComputeCreateInfo info{};
			Shader shader({ "../VulkanHelper/src/VulkanHelper/Shaders/EnvToCubemap.glsl" , VK_SHADER_STAGE_COMPUTE_BIT, {}, true });
			info.Shader = &shader;

			// Descriptor set layouts for the pipeline
			info.DescriptorSetLayouts = {
				imageLayout.GetDescriptorSetLayoutHandle()
			};
			info.debugName = "Env To Cubemap Pipeline";

			// Create the graphics pipeline
			m_EnvToCubemapPipeline.Init(info);
		}
	}

	void Renderer::CreateDescriptorSets()
	{

	}

	// TODO description
	void Renderer::WriteToFile(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height)
	{
		unsigned error = lodepng::encode(filename, image, width, height);

		//if there's an error, display it
		VK_CORE_ASSERT(!error, "encoder error {} | {}", error, lodepng_error_text(error));
	}

	void Renderer::InitImGui()
	{
#ifdef VL_IMGUI

		static bool imGuiInitialized = false;
		if (!imGuiInitialized)
		{
			float resWidth, resHeight;
			glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &resWidth, &resHeight);

			float scale = glm::min(resWidth, resHeight);

			// ImGui Creation
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			io.Fonts->Clear();

			ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.0f * scale);
			if (font == NULL)
				font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttf", 16.0f * scale); // Windows 7

			VK_CORE_ASSERT(font != nullptr, "Couldn't load font C:\\Windows\\Fonts\\msyh.ttc/ttf!");

			ImGui_ImplGlfw_InitForVulkan(m_Context.Window->GetGLFWwindow(), true);
			ImGui_ImplVulkan_InitInfo info{};
			info.Instance = Instance::Get()->GetHandle();
			info.PhysicalDevice = Device::GetPhysicalDevice();
			info.Device = Device::GetDevice();
			info.Queue = Device::GetGraphicsQueue();
			info.DescriptorPool = m_Pool->GetDescriptorPoolHandle();
			info.Subpass = 0;
			info.MinImageCount = 2;
			info.ImageCount = m_Swapchain->GetImageCount();
			info.CheckVkResultFn = CheckVkResult;
			ImGui_ImplVulkan_Init(&info, m_Swapchain->GetSwapchainRenderPass());

			VkCommandBuffer cmdBuffer;
			Device::BeginSingleTimeCommands(cmdBuffer, Device::GetGraphicsCommandPool());
			ImGui_ImplVulkan_CreateFontsTexture(cmdBuffer);
			Device::EndSingleTimeCommands(cmdBuffer, Device::GetGraphicsQueue(), Device::GetGraphicsCommandPool());

			vkDeviceWaitIdle(Device::GetDevice());
			ImGui_ImplVulkan_DestroyFontUploadObjects();

			SetStyle();
			ImGuiStyle& style = ImGui::GetStyle();
			style.ScaleAllSizes(scale);
			imGuiInitialized = true;
		}
#endif
	}

	void Renderer::DestroyImGui()
	{
		//ImGui_ImplVulkan_DestroyDeviceObjects();
		//ImGui_ImplVulkan_Shutdown();
		//ImGui_ImplGlfw_Shutdown();
		//ImGui::DestroyContext();
	}

	void Renderer::SetStyle()
	{
		// Colors
		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
		colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.00f, 0.00f, 0.80f);
		colors[ImGuiCol_Button] = ImVec4(0.64f, 0.00f, 0.00f, 0.54f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.87f, 0.00f, 0.00f, 0.54f);
		colors[ImGuiCol_ButtonActive] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
		colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
		colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_DockingPreview] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
		colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
		colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowPadding = ImVec2(8.00f, 8.00f);
		style.FramePadding = ImVec2(5.00f, 2.00f);
		style.CellPadding = ImVec2(6.00f, 6.00f);
		style.ItemSpacing = ImVec2(6.00f, 6.00f);
		style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
		style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
		style.IndentSpacing = 25;
		style.ScrollbarSize = 15;
		style.GrabMinSize = 10;
		style.WindowBorderSize = 1;
		style.ChildBorderSize = 1;
		style.PopupBorderSize = 1;
		style.FrameBorderSize = 1;
		style.TabBorderSize = 1;
		style.WindowRounding = 7;
		style.ChildRounding = 4;
		style.FrameRounding = 3;
		style.PopupRounding = 4;
		style.ScrollbarRounding = 9;
		style.GrabRounding = 3;
		style.LogSliderDeadzone = 4;
		style.TabRounding = 4;
	}

	/**
	 * @brief Retrieves the current command buffer for rendering.
	 *
	 * @return The current command buffer.
	 */
	VkCommandBuffer Renderer::GetCurrentCommandBuffer()
	{
		VK_CORE_ASSERT(m_IsFrameStarted, "Cannot get command buffer when frame is not in progress");
		return m_CommandBuffers[m_CurrentImageIndex];
	}

	/**
	 * @brief Retrieves the index of the current frame in progress.
	 *
	 * @return The index of the current frame.
	 */
	int Renderer::GetFrameIndex()
	{
		VK_CORE_ASSERT(m_IsFrameStarted, "Cannot get frame index when frame is not in progress");
		return m_CurrentFrameIndex;
	}
}