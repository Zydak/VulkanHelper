#include "Pch.h"

#include "Image.h"
#include "Logger/Logger.h"
#include "Buffer.h"

VulkanHelper::Image::Image(const CreateInfo& createInfo)
{
	m_Device = createInfo.Device;
	m_Usage = createInfo.Usage;
	m_MemoryProperties = createInfo.Properties;
	m_Allocation = new VmaAllocation();
	m_Size.width = createInfo.Width;
	m_Size.height = createInfo.Height;

	m_MipLevels = createInfo.MipMapCount;
	m_Format = createInfo.Format;
	m_Aspect = createInfo.Aspect;
	m_ViewType = createInfo.ViewType;

	CreateImage();
	CreateImageView();
}

VulkanHelper::Image::~Image()
{
	vkDestroyImageView(m_Device->GetHandle(), m_ViewHandle, nullptr);
	vmaDestroyImage(m_Device->GetAllocator(), m_ImageHandle, *m_Allocation);
}

void VulkanHelper::Image::TransitionImageLayout(Device* device, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkCommandBuffer cmdBuffer /*= 0*/, const VkImageSubresourceRange& subresourceRange /*= { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } */)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

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

void VulkanHelper::Image::TransitionImageLayout(VkImageLayout newLayout, VkCommandBuffer cmdBuffer /*= 0*/, uint32_t baseLayer /*= 0*/, uint32_t layerCount /*= 1*/)
{
	if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED)
		return;

	VkCommandBuffer commandBuffer;

	if (!cmdBuffer)
		m_Device->BeginSingleTimeCommands(&commandBuffer, m_Device->GetGraphicsCommandPool()->GetHandle());
	else
		commandBuffer = cmdBuffer;

	VkAccessFlags srcAccess = 0;
	VkAccessFlags dstAccess = 0;
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;

	switch (m_Layout)
	{
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		srcAccess |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		srcStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		break;
	case VK_IMAGE_LAYOUT_GENERAL:
		srcAccess |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		srcStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		srcAccess |= VK_ACCESS_SHADER_READ_BIT;
		srcStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		srcAccess |= VK_ACCESS_TRANSFER_READ_BIT;
		srcStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		srcAccess |= VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		srcAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		break;
	default:
		break;
	}

	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_GENERAL:
		dstAccess |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
		dstStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		dstAccess |= VK_ACCESS_SHADER_READ_BIT;
		dstStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		dstAccess |= VK_ACCESS_TRANSFER_READ_BIT;
		dstStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		dstAccess |= VK_ACCESS_TRANSFER_WRITE_BIT;
		dstStage |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	default:
		break;
	}

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = m_Layout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
	barrier.image = m_ImageHandle;
	VkImageSubresourceRange range{};
	range.aspectMask = m_Aspect;
	range.baseArrayLayer = baseLayer;
	range.layerCount = layerCount;
	range.baseMipLevel = 0;
	range.levelCount = m_MipLevels;
	barrier.subresourceRange = range;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;
	VkPipelineStageFlags srcStageMask = srcStage;
	VkPipelineStageFlags dstStageMask = dstStage;

	vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	if (!cmdBuffer)
		m_Device->EndSingleTimeCommands(commandBuffer, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());

	m_Layout = newLayout;
}

void VulkanHelper::Image::WritePixels(void* data, uint64_t dataSize, bool generateMipMaps /*= false*/, uint64_t offset /*= 0*/, VkCommandBuffer cmd /*= 0*/, uint32_t baseLayer /*= 0*/)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	if (!cmd)
		m_Device->BeginSingleTimeCommands(&commandBuffer, m_Device->GetGraphicsCommandPool()->GetHandle());
	else
		commandBuffer = cmd;

	uint64_t pixelSize = (uint64_t)FormatToSize(m_Format);
	VkDeviceSize imageSize = (uint64_t)m_Size.width * (uint64_t)m_Size.height * pixelSize;

	Buffer::CreateInfo BufferInfo{};
	BufferInfo.BufferSize = imageSize;
	BufferInfo.UsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	BufferInfo.MemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	Buffer buffer(BufferInfo);

	auto res = buffer.Map(imageSize);
	buffer.WriteToBuffer((void*)data, dataSize, offset * pixelSize);
	(void)buffer.Flush();
	buffer.Unmap();

	TransitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd, baseLayer);
	CopyBufferToImage(buffer.GetHandle(), baseLayer, cmd);

	if (generateMipMaps)
		GenerateMipmaps(cmd);

	if (!cmd)
		m_Device->EndSingleTimeCommands(commandBuffer, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());
}

void VulkanHelper::Image::GenerateMipmaps(VkCommandBuffer commandBuffer) const
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = m_ImageHandle;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = (int32_t)m_Size.width;
	int32_t mipHeight = (int32_t)m_Size.height;

	for (uint32_t i = 0; i < m_MipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; // Since this function is called only from WritePixels(...) image will always be in DST_OPTIMAL layout
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i + 1;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			m_ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR
		);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}
}

void VulkanHelper::Image::CreateImageView()
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_ImageHandle;
	viewInfo.viewType = m_ViewType;
	viewInfo.format = m_Format;
	viewInfo.subresourceRange.aspectMask = m_Aspect;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = m_MipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = m_LayerCount;
	VH_CHECK(vkCreateImageView(m_Device->GetHandle(), &viewInfo, nullptr, &m_ViewHandle) == VK_SUCCESS,
		"failed to create image view layer!"
	);
}

void VulkanHelper::Image::CopyBufferToImage(VkBuffer buffer, uint32_t baseLayer, VkCommandBuffer cmd, VkDeviceSize bufferOffset, VkOffset2D offset)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	if (!cmd)
		m_Device->BeginSingleTimeCommands(&commandBuffer, m_Device->GetGraphicsCommandPool()->GetHandle());
	else
		commandBuffer = cmd;

	VkBufferImageCopy region{};
	region.bufferOffset = bufferOffset;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = baseLayer;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { offset.x, offset.y, 0 };
	region.imageExtent = { m_Size.width - offset.x, m_Size.height - offset.y, 1 };

	vkCmdCopyBufferToImage(cmd, buffer, m_ImageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (!cmd)
		m_Device->EndSingleTimeCommands(commandBuffer, m_Device->GetGraphicsQueue(), m_Device->GetGraphicsCommandPool()->GetHandle());
}

void VulkanHelper::Image::CreateImage()
{
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = m_Size.width;
	imageCreateInfo.extent.height = m_Size.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = m_MipLevels;
	imageCreateInfo.arrayLayers = m_LayerCount;
	imageCreateInfo.format = m_Format;
	imageCreateInfo.tiling = m_Tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = m_Usage;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (m_ViewType == VK_IMAGE_VIEW_TYPE_CUBE || m_ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
		imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	(void)m_Device->CreateImage(&m_ImageHandle, m_Allocation, imageCreateInfo, m_MemoryProperties);
}

uint32_t VulkanHelper::Image::FormatToSize(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_R8_UNORM:
		return 1 * 1;
		break;
	case VK_FORMAT_R8_SRGB:
		return 1 * 1;
		break;
	case VK_FORMAT_R8G8_UNORM:
		return 2 * 1;
		break;
	case VK_FORMAT_R8G8_SRGB:
		return 2 * 1;
		break;
	case VK_FORMAT_R8G8B8_UNORM:
		return 3 * 1;
		break;
	case VK_FORMAT_R8G8B8_SRGB:
		return 3 * 1;
		break;
	case VK_FORMAT_B8G8R8_UNORM:
		return 3 * 1;
		break;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4 * 1;
		break;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return 4 * 1;
		break;
	case VK_FORMAT_B8G8R8A8_SRGB:
		return 4 * 1;
		break;
	case VK_FORMAT_R16_UNORM:
		return 1 * 2;
		break;
	case VK_FORMAT_R16_SFLOAT:
		return 1 * 2;
		break;
	case VK_FORMAT_R16G16_UNORM:
		return 2 * 2;
		break;
	case VK_FORMAT_R16G16_SFLOAT:
		return 2 * 2;
		break;
	case VK_FORMAT_R16G16B16_UNORM:
		return 3 * 2;
		break;
	case VK_FORMAT_R16G16B16_SFLOAT:
		return 3 * 2;
		break;
	case VK_FORMAT_R16G16B16A16_UNORM:
		return 4 * 2;
		break;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * 2;
		break;
	case VK_FORMAT_R32_SFLOAT:
		return 1 * 4;
		break;
	case VK_FORMAT_R32G32_SFLOAT:
		return 2 * 4;
		break;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return 3 * 4;
		break;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * 4;
		break;
	default:
		VH_ASSERT(false, "Unsupported format! Format: {}", (int)format);
		break;
	}

	return 0; // just to get rid of the warning
}