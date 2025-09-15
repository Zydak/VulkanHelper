#include "ImageImpl.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "Vulkan/CommandBuffer.h"
#include "CommandBufferImpl.h"
#include "BufferImpl.h"

namespace VulkanHelper
{
    static uint32_t GetTexelSizeInBytes(VkFormat format)
    {
        switch (format) {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_SRGB:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
                return 1;

            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_SRGB:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
                return 2;

            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SFLOAT:
                return 4;

            case VK_FORMAT_R32G32_SFLOAT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R32G32_UINT:
                return 8;

            case VK_FORMAT_R32G32B32A32_SFLOAT:
            case VK_FORMAT_R32G32B32A32_UINT:
                return 16;

            default:
                VH_ASSERT(false, "Unrecognized format!");
        }
    }

    Expected<SharedPtr<Image::Impl>, VHResult> Image::Impl::New(
        const SharedPtr<Device::Impl>& device,
        uint32_t height,
        uint32_t width,
        uint32_t mipCount,
        uint32_t layerCount,
        Format format,
        Usage usage,
        Tiling tiling,
        Aspect aspect,
        Layout initialLayout,
        SampleCount sampleCount,
        bool usePersistentStagingBuffer,
        bool allowMapping
    )
    {
        VH_LOG_INFO("Creating Image Implementation");

        if (device == nullptr)
        {
            VH_LOG_ERROR("Device Can't be nullptr!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (width == 0)
        {
            VH_LOG_ERROR("Image width must be greater than 0!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (height == 0)
        {
            VH_LOG_ERROR("Image height must be greater than 0!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (layerCount == 0)
        {
            VH_LOG_ERROR("Image layer count must be greater than 0!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (mipCount == 0)
        {
            VH_LOG_ERROR("Image mip count must be greater than 0!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (format == Format::UNDEFINED)
        {
            VH_LOG_ERROR("Image format must be specified!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (usage == Usage::UNDEFINED)
        {
            VH_LOG_ERROR("Image usage flags must be specified!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        if (allowMapping && tiling != Tiling::LINEAR)
        {
            VH_LOG_ERROR("If you want to map the image (AllowMapping = true) Tiling has to be set to LINEAR");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VkImageCreateInfo imageCreateInfo{};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.extent.width = width;
        imageCreateInfo.extent.height = height;
        imageCreateInfo.extent.depth = 1;
        imageCreateInfo.mipLevels = mipCount;
        imageCreateInfo.arrayLayers = layerCount;
        imageCreateInfo.format = (VkFormat)format;
        imageCreateInfo.tiling = (VkImageTiling)tiling;
        imageCreateInfo.initialLayout = (VkImageLayout)initialLayout;
        imageCreateInfo.usage = (VkImageUsageFlags)usage;
        imageCreateInfo.samples = (VkSampleCountFlagBits)sampleCount;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto image = device->AllocateImage(imageCreateInfo, allowMapping);
        if (!image.HasValue())
        {
            VH_LOG_ERROR("Failed to allocate Image!");
            return Unexpected(image.Error());
        }

        VulkanMemoryAllocator::BufferAllocation staginBuffer{};
        if (usePersistentStagingBuffer)
        {
            uint32_t texelSize = GetTexelSizeInBytes((VkFormat)format);
            uint64_t imageSize = width * height * texelSize;

            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = imageSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            auto stagingBufferResult = device->AllocateBuffer(bufferInfo, true);
            if (!stagingBufferResult.HasValue())
            {
                VH_LOG_ERROR("Failed to create staging buffer for image upload");
                device->DeallocateImage(image.Value());
                return Unexpected(stagingBufferResult.Error());
            }

            staginBuffer = stagingBufferResult.Value();
        }

        return SharedPtr<Impl>(std::make_shared<Impl>(Impl(
            device,
            format,
            Vector<Layout>(layerCount, initialLayout), // Initialize all layers with the same layout
            aspect, 
            width,
            height,
            layerCount,
            mipCount,
            allowMapping,
            Move(image.Value()),
            Move(staginBuffer)
        )));
    }

    void Image::Impl::TransitionImageLayout(Layout newLayout, const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t baseLayer, uint32_t layerCount)
    {
        bool allLayoutsAreTheSame = true;
        for (uint32_t i = baseLayer; i < baseLayer + layerCount; ++i)
        {
            if (m_Layout[i] != newLayout)
            {
                allLayoutsAreTheSame = false;
                break;
            }
        }
        if (allLayoutsAreTheSame)
            return;

        VkAccessFlags srcAccess = 0;
        VkAccessFlags dstAccess = 0;
        VkPipelineStageFlags srcStage = 0;
        VkPipelineStageFlags dstStage = 0;

        switch ((VkImageLayout)m_Layout[baseLayer])
        {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            srcAccess |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            srcStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            srcAccess |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            srcStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            srcAccess |= VK_ACCESS_SHADER_READ_BIT;
            srcStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
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
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            srcAccess |= 0;
            srcStage |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            srcAccess = 0;
            srcStage = 0;
            break;
        default:
            VH_ASSERT(false, "Transition for your src layout is not implemented yet!");
            break;
        }

        switch ((VkImageLayout)newLayout)
        {
        case VK_IMAGE_LAYOUT_GENERAL:
            dstAccess |= VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
            dstStage |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            dstAccess |= VK_ACCESS_SHADER_READ_BIT;
            dstStage |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
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
            dstAccess |= 0;
            dstStage |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            VH_ASSERT(false, "Trying to convert to layout undefined, Don't do that!");
            break;
        default:
            VH_ASSERT(false, "Transition for your dst layout is not implemented yet!");
            break;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = (VkImageLayout)m_Layout[baseLayer];
        barrier.newLayout = (VkImageLayout)newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        barrier.image = m_Allocation.image;
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

        vkCmdPipelineBarrier(commandBuffer->GetCommandBuffer(), srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        for (uint32_t i = baseLayer; i < baseLayer + layerCount; ++i)
        {
            m_Layout[i] = newLayout;
        }
    }

    void Image::Impl::Barrier(const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t baseLayer, uint32_t layerCount, AccessFlags srcAccessMask, AccessFlags dstAccessMask, PipelineStages srcStage, PipelineStages dstStage)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = (VkImageLayout)m_Layout[baseLayer];
        barrier.newLayout = (VkImageLayout)m_Layout[baseLayer];
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
        barrier.image = m_Allocation.image;
        VkImageSubresourceRange range{};
        range.aspectMask = (VkImageAspectFlags)m_Aspect;
        range.baseArrayLayer = baseLayer;
        range.layerCount = layerCount;
        range.baseMipLevel = 0;
        range.levelCount = m_MipCount;
        barrier.subresourceRange = range;
        barrier.srcAccessMask = (VkAccessFlags)srcAccessMask;
        barrier.dstAccessMask = (VkAccessFlags)dstAccessMask;
        VkPipelineStageFlags srcStageMask = (VkPipelineStageFlags)srcStage;
        VkPipelineStageFlags dstStageMask = (VkPipelineStageFlags)dstStage;

        vkCmdPipelineBarrier(commandBuffer->GetCommandBuffer(), srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    Image::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Format(other.m_Format)
        , m_Layout(Move(other.m_Layout))
        , m_Aspect(other.m_Aspect)
        , m_Width(other.m_Width)
        , m_Height(other.m_Height)
        , m_LayerCount(other.m_LayerCount)
        , m_MipCount(other.m_MipCount)
        , m_Mapable(other.m_Mapable)
        , m_Allocation(Move(other.m_Allocation))
        , m_StagingBufferAllocation(Move(other.m_StagingBufferAllocation))
    {
        other.m_Device = nullptr;
        other.m_Allocation.Allocation = nullptr;
        other.m_StagingBufferAllocation.Allocation = nullptr;
        other.m_Allocation.image = VK_NULL_HANDLE;
    }

    Image::Impl& Image::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (m_Allocation.Allocation != nullptr)
        {
            VH_LOG_INFO("Destroying Image Implementation");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Allocation);
            m_Allocation.Allocation = nullptr;
            m_Allocation.image = VK_NULL_HANDLE;
        }
        if (m_StagingBufferAllocation.Allocation != nullptr)
        {
            m_Device->GetDeleteQueue().QueueForDeletion(m_StagingBufferAllocation);
        }

        m_Device = other.m_Device;
        m_Format = other.m_Format;
        m_Layout = Move(other.m_Layout);
        m_Aspect = other.m_Aspect;
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_LayerCount = other.m_LayerCount;
        m_MipCount = other.m_MipCount;
        m_Mapable = other.m_Mapable;
        m_Allocation = Move(other.m_Allocation);
        m_StagingBufferAllocation = Move(other.m_StagingBufferAllocation);

        other.m_Device = nullptr;
        other.m_Allocation.Allocation = nullptr;
        other.m_StagingBufferAllocation.Allocation = nullptr;
        other.m_Allocation.image = VK_NULL_HANDLE;

        return *this;
    }

    Image::Impl::~Impl()
    {
        if (m_Allocation.Allocation != nullptr)
        {
            VH_LOG_INFO("Destroying Image Implementation");
            m_Device->GetDeleteQueue().QueueForDeletion(m_Allocation);
            m_Allocation.Allocation = nullptr;
            m_Allocation.image = VK_NULL_HANDLE;
        }
        if (m_StagingBufferAllocation.Allocation != nullptr)
        {
            m_Device->GetDeleteQueue().QueueForDeletion(m_StagingBufferAllocation);
        }
    }

    Expected<void*, VHResult> Image::Impl::Map()
    {
        if (!m_Mapable)
        {
            VH_LOG_ERROR("Attempting to map non-mappable image!");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        auto mapResult = m_Device->MapMemory(m_Allocation.Allocation);
        if (!mapResult.HasValue())
        {
            VH_LOG_ERROR("Failed to map image memory!");
            return Unexpected(mapResult.Error());
        }

        return mapResult.Value();
    }

    void Image::Impl::Unmap()
    {
        if (!m_Mapable)
        {
            VH_LOG_ERROR("Attempting to unmap non-mappable image!");
            return;
        }

        m_Device->UnmapMemory(m_Allocation.Allocation);
    }

    VHResult Image::Impl::CopyFromBuffer(
        const SharedPtr<CommandBuffer::Impl> commandBuffer,
        const SharedPtr<Buffer::Impl>& src,
        uint32_t bufferOffset,
        uint32_t imageOffsetX,
        uint32_t imageOffsetY,
        uint32_t imageExtentX,
        uint32_t imageExtentY,
        uint32_t imageBaseLayer,
        uint32_t layerCount
    )
    {
        return src->CopyToImage(commandBuffer, SharedPtr<Image::Impl>(this), bufferOffset, imageOffsetX, imageOffsetY, imageExtentX, imageExtentY, imageBaseLayer, layerCount);
    }

    VHResult Image::Impl::CopyToBuffer(
        const SharedPtr<CommandBuffer::Impl> commandBuffer,
        const SharedPtr<Buffer::Impl>& dst,
        uint32_t bufferOffset,
        uint32_t imageOffsetX,
        uint32_t imageOffsetY,
        uint32_t imageExtentX,
        uint32_t imageExtentY,
        uint32_t imageBaseLayer,
        uint32_t layerCount
    )
    {
        return dst->CopyFromImage(commandBuffer, SharedPtr<Image::Impl>(const_cast<Image::Impl*>(this)), bufferOffset, imageOffsetX, imageOffsetY, imageExtentX, imageExtentY, imageBaseLayer, layerCount);
    }

    VHResult Image::Impl::CopyFromImage(const Image& srcImage, const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t srcBaseLayer, uint32_t dstBaseLayer, uint32_t layerCount)
    {
        VkCommandBuffer vkCmd = commandBuffer->GetCommandBuffer();

        VkImageCopy copyRegion{};
        copyRegion.srcSubresource.aspectMask = static_cast<VkImageAspectFlags>(m_Aspect);
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.baseArrayLayer = srcBaseLayer;
        copyRegion.srcSubresource.layerCount = layerCount;
        copyRegion.srcOffset = {0, 0, 0};
        
        copyRegion.dstSubresource.aspectMask = static_cast<VkImageAspectFlags>(m_Aspect);
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.baseArrayLayer = dstBaseLayer;
        copyRegion.dstSubresource.layerCount = layerCount;
        copyRegion.dstOffset = {0, 0, 0};
        
        copyRegion.extent.width = m_Width;
        copyRegion.extent.height = m_Height;
        copyRegion.extent.depth = 1;

        vkCmdCopyImage(vkCmd, srcImage.m_Impl->m_Allocation.image, (VkImageLayout)srcImage.m_Impl->GetLayout(srcBaseLayer),
                       m_Allocation.image, (VkImageLayout)m_Layout[dstBaseLayer], 1, &copyRegion);

        return VHResult::OK;
    }

    VHResult Image::Impl::BlitFromImage(const Image& srcImage, const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t srcBaseLayer, uint32_t dstBaseLayer, uint32_t layerCount)
    {
        VkCommandBuffer vkCmd = commandBuffer->GetCommandBuffer();

        VkImageBlit blitRegion{};
        blitRegion.srcSubresource.aspectMask = static_cast<VkImageAspectFlags>(srcImage.m_Impl->GetAspect());
        blitRegion.srcSubresource.mipLevel = 0;
        blitRegion.srcSubresource.baseArrayLayer = srcBaseLayer;
        blitRegion.srcSubresource.layerCount = layerCount;
        blitRegion.srcOffsets[0] = {0, 0, 0};
        blitRegion.srcOffsets[1] = {static_cast<int32_t>(srcImage.GetWidth()), static_cast<int32_t>(srcImage.GetHeight()), 1};

        blitRegion.dstSubresource.aspectMask = static_cast<VkImageAspectFlags>(m_Aspect);
        blitRegion.dstSubresource.mipLevel = 0;
        blitRegion.dstSubresource.baseArrayLayer = dstBaseLayer;
        blitRegion.dstSubresource.layerCount = layerCount;
        blitRegion.dstOffsets[0] = {0, 0, 0};
        blitRegion.dstOffsets[1] = {static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height), 1};

        vkCmdBlitImage(vkCmd, srcImage.m_Impl->m_Allocation.image, (VkImageLayout)srcImage.m_Impl->GetLayout(srcBaseLayer),
                       m_Allocation.image, (VkImageLayout)m_Layout[dstBaseLayer], 1, &blitRegion,
                       VK_FILTER_LINEAR);

        return VHResult::OK;
    }

    [[nodiscard]] uint64_t Image::Impl::GetSizeInBytes() const
    {
        return m_Width * m_Height * GetTexelSizeInBytes((VkFormat)m_Format);
    }

    //
    //  Forward Functions
    //
    VulkanHelper::Expected<Image, VHResult> Image::New(const Image::Config& config)
    {
        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            config.Height,
            config.Width,
            config.MipCount,
            config.LayerCount,
            config.Format,
            config.Usage,
            config.Tiling,
            config.Aspect,
            config.InitialLayout,
            config.SampleCount,
            config.UsePersistentStagingBuffer,
            config.AllowMapping
        );

        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Image::Impl::CreatePublicInterface(implResult.Value());
    }

    Image::Image()
        : m_Impl(nullptr)
    {
    }

    Image::Image(const Image& other)
        : m_Impl(other.m_Impl)
    {
    }

    Image& Image::operator=(const Image& other)
    {
        if (this == &other)
            return *this;

        m_Impl = other.m_Impl;

        return *this;
    }

    Image::Image(Image&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Image& Image::operator=(Image&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Image::~Image()
    {

    }

    Image::Image(const VulkanHelper::SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }

    void Image::TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount)
    {
        m_Impl->TransitionImageLayout(newLayout, CommandBuffer::Impl::GetImplementation(commandBuffer), baseLayer, layerCount);
    }

    [[nodiscard]] Format Image::GetFormat() const { return m_Impl->GetFormat(); }
    [[nodiscard]] Image::Layout Image::GetLayout() const { return m_Impl->GetLayout(); }
    [[nodiscard]] Image::Aspect Image::GetAspect() const { return m_Impl->GetAspect(); }
        
    [[nodiscard]] uint32_t Image::GetWidth() const { return m_Impl->GetWidth(); }
    [[nodiscard]] uint32_t Image::GetHeight() const { return m_Impl->GetHeight(); }
    [[nodiscard]] uint32_t Image::GetLayerCount() const { return m_Impl->GetLayerCount(); }
    [[nodiscard]] uint32_t Image::GetMipCount() const { return m_Impl->GetMipCount(); }

    Expected<void*, VHResult> Image::Map()
    {
        return m_Impl->Map();
    }

    void Image::Unmap()
    {
        return m_Impl->Unmap();
    }

    VHResult Image::CopyFromImage(const Image& srcImage, CommandBuffer& commandBuffer, uint32_t srcBaseLayer, uint32_t dstBaseLayer, uint32_t layerCount)
    {
        return m_Impl->CopyFromImage(srcImage, CommandBuffer::Impl::GetImplementation(commandBuffer), srcBaseLayer, dstBaseLayer, layerCount);
    }

    VHResult Image::BlitFromImage(const Image& srcImage, CommandBuffer& commandBuffer, uint32_t srcBaseLayer, uint32_t dstBaseLayer, uint32_t layerCount)
    {
        return m_Impl->BlitFromImage(srcImage, CommandBuffer::Impl::GetImplementation(commandBuffer), srcBaseLayer, dstBaseLayer, layerCount);
    }

    void Image::Barrier(CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount, AccessFlags srcAccessMask, AccessFlags dstAccessMask, PipelineStages srcStage, PipelineStages dstStage)
    {
        m_Impl->Barrier(CommandBuffer::Impl::GetImplementation(commandBuffer), baseLayer, layerCount, srcAccessMask, dstAccessMask, srcStage, dstStage);
    }
}