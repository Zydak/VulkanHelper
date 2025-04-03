#pragma once

#include "Pch.h"
#include "Pipeline.h"
#include "Image.h"
#include "Buffer.h"
#include "Descriptors/DescriptorSetLayout.h"

namespace VulkanHelper
{
	class Device;

	class DeleteQueue
	{
	public:
		DeleteQueue() = delete;
		~DeleteQueue() = delete;

		struct CreateInfo
		{
			Device* Device = nullptr;
			uint32_t FramesInFlight;
		};

		static void Init(const CreateInfo& info);
		static void Destroy();

		static void ClearQueue();

		static void UpdateQueue();

		static void DeletePipeline(const Pipeline& pipeline);
		static void DeleteImage(Image& image);
		static void DeleteBuffer(Buffer& buffer);
		static void DeleteDescriptorSetLayout(DescriptorSetLayout& set);
	private:

		struct PipelineInfo
		{
			VkPipeline Handle;
			VkPipelineLayout Layout;
		};

		struct ImageInfo
		{
			VkImage Handle;
			VkImageView View;
			VmaAllocation* Allocation;
		};

		struct BufferInfo
		{
			VkBuffer Handle;
			VmaAllocation* Allocation;
		};

		inline static Device* s_Device = nullptr;
		inline static uint32_t s_FramesInFlight = 0;

		inline static std::vector<std::pair<PipelineInfo, uint32_t>> s_PipelineQueue;
		inline static std::vector<std::pair<ImageInfo, uint32_t>> s_ImageQueue;
		inline static std::vector<std::pair<BufferInfo, uint32_t>> s_BufferQueue;
		inline static std::vector<std::pair<VkDescriptorSetLayout, uint32_t>> s_SetQueue;

		inline static std::mutex s_Mutex;
	};
}