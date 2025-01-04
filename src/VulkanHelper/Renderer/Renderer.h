#pragma once
#include "pch.h"
#include "../Utility/Utility.h"

#include "Vulkan/Swapchain.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/DescriptorSet.h"
#include "Vulkan/PushConstant.h"
#include "Vulkan/SBT.h"
#include "Mesh.h"

#include "Core/Context.h"

#include <vulkan/vulkan.h>

#ifndef VL_IMGUI
#define VL_IMGUI
#endif

namespace VulkanHelper
{
	class Renderer
	{
	public:
		~Renderer() = default;
		Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&& other) noexcept;
		Renderer& operator=(Renderer&& other) noexcept;

		void Init(const VulkanHelperContext& context, uint32_t maxFramesInFlight);
		void Destroy();
		
		inline Swapchain& GetSwapchain() { return *m_Swapchain; }
		inline DescriptorPool& GetDescriptorPool() { return *m_Pool; }
		inline Sampler& GetLinearSampler() { return m_RendererLinearSampler; }
		inline Sampler& GetLinearRepeatSampler() { return m_RendererLinearSamplerRepeat; }
		inline Sampler& GetNearestSampler() { return m_RendererNearestSampler; }
		inline uint32_t& GetCurrentFrameIndex() { return m_CurrentFrameIndex; }
		inline Mesh& GetQuadMesh() { return m_QuadMesh; }
		inline float GetAspectRatio() { return m_Swapchain->GetExtentAspectRatio(); }
		inline VkExtent2D GetExtent() { return m_Swapchain->GetSwapchainExtent(); }

		bool BeginFrame();
		void EndFrame();

		void SetImGuiFunction(std::function<void()> fn);
		void SetStyle();
		void RayTrace(VkCommandBuffer cmdBuf, SBT* sbt, VkExtent2D imageSize, uint32_t depth = 1);

		VkCommandBuffer GetCurrentCommandBuffer();
		int GetFrameIndex();
		int GetImageIndex() const { return m_CurrentImageIndex; };

		void SaveImageToFile(const std::string& filepath, Image* image);
		void ImGuiPass();
		void FramebufferCopyPassImGui(DescriptorSet* descriptorWithImageSampler);
		void FramebufferCopyPassBlit(Image* image);
		void EnvMapToCubemapPass(Ref<Image> envMap, Ref<Image> cubemap);

		void BeginRenderPass(const std::vector<VkClearValue>& clearColors, VkFramebuffer framebuffer, VkRenderPass renderPass, VkExtent2D extent);
		void EndRenderPass();

		inline uint32_t GetMaxFramesInFlight() const { return m_MaxFramesInFlight; }

		inline bool IsInitialized() const { return m_Initialized; }

	private:
		void RecreateSwapchain();
		void CreateCommandBuffers();
		void CreatePool();
		void CreatePipeline();
		void CreateDescriptorSets();
		void WriteToFile(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height);

		void InitImGui();
		void DestroyImGui();

		uint32_t m_MaxFramesInFlight = 0;

		Scope<DescriptorPool> m_Pool = nullptr;
		VulkanHelperContext m_Context;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		Scope<Swapchain> m_Swapchain = nullptr;

		bool m_IsFrameStarted = false;
		uint32_t m_CurrentImageIndex = 0;
		uint32_t m_CurrentFrameIndex = 0;

		Mesh m_QuadMesh;
		Sampler m_RendererLinearSampler;
		Sampler m_RendererLinearSamplerRepeat;
		Sampler m_RendererNearestSampler;

		Ref<DescriptorSet> m_EnvToCubemapDescriptorSet = nullptr;

		Pipeline m_HDRToPresentablePipeline;
		Pipeline m_EnvToCubemapPipeline;

		std::function<void()> m_ImGuiFunction;

		bool m_Initialized = false;

		friend class Window;
	};
}