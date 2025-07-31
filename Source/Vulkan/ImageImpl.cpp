#include "ImageImpl.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Vulkan/CommandBuffer.h"
#include "CommandBufferImpl.h"

namespace VulkanHelper
{
    Expected<UniquePtr<Image::Impl>, VHResult> Image::Impl::New(const Image::Config& config)
    {
        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = config.Width;
        imageCreateInfo.extent.height = config.Height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = config.MipCount;
        imageCreateInfo.arrayLayers = config.LayerCount;
        imageCreateInfo.format = (VkFormat)config.Format;
        imageCreateInfo.tiling = (VkImageTiling)config.Tiling;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.usage = (VkImageUsageFlags)config.Usage;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        (void) imageCreateInfo;

        return Unexpected(VHResult::NOT_IMPLEMENTED);
    }

    void Image::Impl::TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount)
    {
        if (m_Layout == newLayout)
            return;

        VkAccessFlags srcAccess = 0;
        VkAccessFlags dstAccess = 0;
        VkPipelineStageFlags srcStage = 0;
        VkPipelineStageFlags dstStage = 0;

        switch ((VkImageLayout)m_Layout)
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
        case VK_IMAGE_LAYOUT_UNDEFINED:
            srcAccess = 0;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        default:
            VH_ASSERT(false, "Transition for your src layout is not implemented yet!");
            break;
        }

        switch ((VkImageLayout)newLayout)
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
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            dstAccess |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dstStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            dstAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dstStage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            dstAccess |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dstStage |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            dstAccess |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dstStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        default:
            VH_ASSERT(false, "Transition for your dst layout is not implemented yet!");
            break;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = (VkImageLayout)m_Layout;
        barrier.newLayout = (VkImageLayout)newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        barrier.image = m_Image;
        VkImageSubresourceRange range{};
        range.aspectMask = (VkImageAspectFlags)m_Aspect;
        range.baseArrayLayer = baseLayer;
        range.layerCount = layerCount;
        range.baseMipLevel = 0;
        range.levelCount = m_MipCount;
        barrier.subresourceRange = range;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        VkPipelineStageFlags srcStageMask = srcStage;
        VkPipelineStageFlags dstStageMask = dstStage;

        vkCmdPipelineBarrier(commandBuffer.m_Impl->GetCommandBuffer(), srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_Layout = newLayout;
    }

    Image::Impl::~Impl()
    {

    }

    //
    //  Forward Functions
    //
    VulkanHelper::Expected<Image, VHResult> Image::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Image{ VulkanHelper::Move(implResult.Value()) };
    }

    Image::Image(Image&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Image& Image::operator=(Image&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Image(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Image::~Image()
    {

    }

    Image::Image(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    void Image::TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount)
    {
        m_Impl->TransitionImageLayout(newLayout, commandBuffer, baseLayer, layerCount);
    }
}