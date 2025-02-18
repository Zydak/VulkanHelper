#include "Pch.h"

#include "Device.h"
#include "Image.h"

void VulkanHelper::Image::TransitionImageLayout(Device* device, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkCommandBuffer cmdBuffer /*= 0*/, const VkImageSubresourceRange& subresourceRange /*= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } */)
{
	VkCommandBuffer commandBuffer;

	if (!cmdBuffer)
		device->BeginSingleTimeCommands(&commandBuffer, device->GetGraphicsCommandPool()->GetHandle());
	else
		commandBuffer = cmdBuffer;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = subresourceRange;

	barrier.dstAccessMask = dstAccess;
	barrier.srcAccessMask = srcAccess;

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	if (!cmdBuffer)
		device->EndSingleTimeCommands(commandBuffer, device->GetGraphicsQueue(), device->GetGraphicsCommandPool()->GetHandle());
}
