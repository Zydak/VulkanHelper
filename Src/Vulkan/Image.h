#pragma once
#include "Pch.h"

#include "vulkan/vulkan_core.h"
#include "ErrorCodes.h"
#include <vk_mem_alloc.h>

namespace VulkanHelper
{
	class Device;

	class Image
	{
	public:
		struct CreateInfo
		{
			Device* Device = nullptr;
			VkFormat Format = VK_FORMAT_MAX_ENUM;
			VkImageUsageFlags Usage = 0;
			VkMemoryPropertyFlags Properties = 0;
			VkImageAspectFlagBits Aspect = VK_IMAGE_ASPECT_NONE;
			VkImageTiling Tiling = VK_IMAGE_TILING_OPTIMAL;
			uint32_t Height = 0;
			uint32_t Width = 0;
			VkImageViewType ViewType = VK_IMAGE_VIEW_TYPE_2D;
			uint32_t LayerCount = 1;
			uint32_t MipMapCount = 0;

			const char* DebugName = "";
		};

		[[nodiscard]] ResultCode Init(const CreateInfo& createInfo);
		Image() = default;
		~Image();

		Image(const Image& other) = delete;
		Image& operator=(const Image& other) = delete;
		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;

		void TransitionImageLayout(VkImageLayout newLayout, VkCommandBuffer cmdBuffer = 0, uint32_t baseLayer = 0, uint32_t layerCount = 1);
		static void TransitionImageLayout(Device* device, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkCommandBuffer cmdBuffer = 0, const VkImageSubresourceRange& subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		void CopyBufferToImage(VkBuffer buffer, uint32_t baseLayer = 0, VkCommandBuffer cmd = 0, VkDeviceSize bufferOffset = 0, VkOffset2D imageOffset = { 0, 0 });
		void CopyImageToImage(VkImage image, uint32_t width, uint32_t height, VkImageLayout layout, VkCommandBuffer cmd, VkOffset3D srcOffset = { 0, 0, 0 }, VkOffset3D dstOffset = { 0, 0, 0 });
		void BlitImageToImage(Image* srcImage, VkCommandBuffer cmd);

		[[nodiscard]] VulkanHelper::ResultCode WritePixels(void* data, uint64_t dataSize, bool generateMipMaps = false, uint64_t offset = 0, VkCommandBuffer cmd = 0, uint32_t baseLayer = 0);
		void GenerateMipmaps(VkCommandBuffer cmd) const;
	public:

		inline VkImage GetImage() const { return m_ImageHandle; }
		inline VkImageView GetImageView() const { return m_ViewHandle; }
		inline VkExtent2D GetImageSize() const { return m_Size; }
		inline VkImageUsageFlags GetUsageFlags() const { return m_Usage; }
		inline VkMemoryPropertyFlags GetMemoryProperties() const { return m_MemoryProperties; }
		inline VkImageLayout GetLayout() const { return m_Layout; }
		inline void SetLayout(VkImageLayout newLayout) { m_Layout = newLayout; }
		inline uint32_t GetMipLevelsCount() const { return m_MipLevels; }
		inline VmaAllocation* GetAllocation() { return m_Allocation; }

	private:

		uint32_t FormatToSize(VkFormat format);
		void CreateImageView();
		ResultCode CreateImage();

		Device* m_Device = nullptr;
		VkFormat m_Format = VK_FORMAT_MAX_ENUM;
		VkImageAspectFlagBits m_Aspect = VK_IMAGE_ASPECT_NONE;
		VkImageTiling m_Tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageViewType m_ViewType = VK_IMAGE_VIEW_TYPE_2D;

		VkImage m_ImageHandle = VK_NULL_HANDLE;
		VkImageView m_ViewHandle = VK_NULL_HANDLE;
		VmaAllocation* m_Allocation;
		VkExtent2D m_Size = { 0, 0 };
		uint32_t m_MipLevels = 0;
		uint32_t m_LayerCount = 1;

		VkImageUsageFlags m_Usage;
		VkMemoryPropertyFlags m_MemoryProperties;
		VkImageLayout m_Layout = VK_IMAGE_LAYOUT_UNDEFINED;

		void Destroy();
		void Move(Image&& other);
		void Reset();
	};

}