#pragma once
#include "Pch.h"

#include "vulkan/vulkan.h"

namespace VulkanHelper
{
	class Device;

	class Image
	{
	public:
		static void TransitionImageLayout(
			Device* device,
			VkImage image,
			VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkPipelineStageFlags srcStage,
			VkPipelineStageFlags dstStage,
			VkAccessFlags srcAccess,
			VkAccessFlags dstAccess,
			VkCommandBuffer cmdBuffer = 0,
			const VkImageSubresourceRange& subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);
	};
}

