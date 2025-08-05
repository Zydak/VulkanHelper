#include "RendererImpl.h"
#include "Core/Error.h"
#include "Log/Log.h"

#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"

#include "../Vulkan/ImageViewImpl.h"
#include "../Vulkan/CommandBufferImpl.h"
#include "../Vulkan/FunctionLoader.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    Expected<UniquePtr<Renderer::Impl>, VHResult> Renderer::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Creating Renderer Implementation");

        auto swapchain = Swapchain::New({config.Device, config.Window, config.FramesInFlight});
        if (!swapchain.HasValue())
        {
            VH_LOG_ERROR("Couldn't create swapchain!");
            return Unexpected(swapchain.Error());
        }

        Device::QueueFamilyIndices queueIndices = config.Device->GetQueueFamilyIndices();
        auto commandPool = CommandPool::New({config.Device, CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, queueIndices.GraphicsFamily});
        if (!commandPool.HasValue())
        {
            VH_LOG_ERROR("Couldn't create CommandPool!");
            return Unexpected(commandPool.Error());
        }

        Vector<CommandBuffer> commandBuffers;
        for (uint32_t i = 0; i < config.FramesInFlight; i++)
        {
            CommandBuffer cmdBuf = commandPool->AllocateCommandBuffer({}).Value();
            commandBuffers.PushBack(Move(cmdBuf)); // Allocate cmdBuffer for each frame
        }

        return UniquePtr(new Impl(config.Device, config.Window, Move(swapchain.Value()), Move(commandPool.Value()), Move(commandBuffers)));
    }

    Renderer::Impl::~Impl()
    {

    }

    Renderer::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Window(other.m_Window)
        , m_Swapchain(VulkanHelper::Move(other.m_Swapchain))
        , m_CommandPool(VulkanHelper::Move(other.m_CommandPool))
        , m_CommandBuffers(VulkanHelper::Move(other.m_CommandBuffers))
    {
        other.m_Device = nullptr;
        other.m_Window = nullptr;
    }

    Renderer::Impl& Renderer::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Cleanup current state

        m_Device = other.m_Device;
        other.m_Device = nullptr;
        m_Window = other.m_Window;
        other.m_Window = nullptr;

        m_Swapchain = Move(other.m_Swapchain);
        m_CommandPool = Move(other.m_CommandPool);
        m_CommandBuffers = Move(other.m_CommandBuffers);

        return *this;
    }

    Expected<CommandBuffer*, VHResult> Renderer::Impl::BeginFrame()
    {
        VHResult res = m_Swapchain.AcquireNextImage();
        if (res != VHResult::OK)
            return Unexpected(res);

        const uint32_t currentFrameIndex = m_Swapchain.GetCurrentFrameIndex();
        res = m_CommandBuffers[currentFrameIndex].BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);

        if (res != VHResult::OK)
            return Unexpected(res);
        
        return &m_CommandBuffers[currentFrameIndex];
    }

    VHResult Renderer::Impl::EndFrame()
    {
        const uint32_t currentFrameIndex = m_Swapchain.GetCurrentFrameIndex();

        m_Swapchain.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::PRESENT_SRC_KHR, m_CommandBuffers[currentFrameIndex], 0, 1);
        VHResult res = m_CommandBuffers[currentFrameIndex].EndRecording();
        if (res != VHResult::OK)
            return res;

        res = m_Swapchain.Submit(m_CommandBuffers[currentFrameIndex]);
        if (res != VHResult::OK)
            return res;

        return VHResult::OK;
    }

    void Renderer::Impl::BeginRendering(
            CommandBuffer& commandBuffer,
            const VulkanHelper::Vector<ImageView*>& targetImagesColor,
            const ImageView* targetImageDepth,
            glm::uvec2 scissorsStart,
            glm::uvec2 scissorsEnd
    )
    {
        // Make sure that all images are the same size
        if (!targetImagesColor.Empty())
        {
            glm::uvec2 prevIndexSize = { targetImagesColor[0]->GetWidth(), targetImagesColor[0]->GetHeight() };
            for (size_t i = 1; i < targetImagesColor.Size(); i++)
            {
                glm::uvec2 currentIndexSize = { targetImagesColor[i]->GetWidth(), targetImagesColor[i]->GetHeight() };
                VH_ASSERT(prevIndexSize.x == currentIndexSize.x && prevIndexSize.y == currentIndexSize.y, "Target Images must be the same size!");
                prevIndexSize = currentIndexSize;
            }

            if (targetImageDepth != nullptr)
            {
                glm::uvec2 depthSize = { targetImageDepth->GetWidth(), targetImageDepth->GetHeight() };
                VH_ASSERT(prevIndexSize.x == depthSize.x && prevIndexSize.y == depthSize.y, "Depth image must be the same size as color images!");
            }
        }

        glm::uvec2 renderSize = {0u, 0u};
        if (!targetImagesColor.Empty())
            renderSize = { targetImagesColor[0]->GetWidth(), targetImagesColor[0]->GetHeight() };
        else
            renderSize = { targetImageDepth->GetWidth(), targetImageDepth->GetHeight() };

        if (scissorsEnd.x == 0 && scissorsEnd.y == 0)
        {
            scissorsEnd = {renderSize.x, renderSize.y};
        }
        VH_ASSERT(scissorsStart.x < scissorsEnd.x && scissorsStart.y < scissorsEnd.y, "ScissorsStart must be smaller than ScissorsEnd!");

        VkViewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)renderSize.x;
        viewport.height = (float)renderSize.y;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{
            {(int)scissorsStart.x, (int)scissorsStart.y},
            {scissorsEnd.x, scissorsEnd.y}
        };

        VkCommandBuffer commandBufferHandle = CommandBuffer::Impl::GetImplementation(&commandBuffer)->GetCommandBuffer();

	    vkCmdSetViewport(commandBufferHandle, 0, 1, &viewport);
	    vkCmdSetScissor(commandBufferHandle, 0, 1, &scissor);

        VulkanHelper::Vector<VkRenderingAttachmentInfo> colorAttachments;
        for (size_t i = 0; i < targetImagesColor.Size(); i++)
        {
            VkRenderingAttachmentInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            info.clearValue = { {{0.1f, 0.1f, 0.1f, 1.0f}} };

            ImageView::Impl* targetImageImpl = ImageView::Impl::GetImplementation(targetImagesColor[i]);
            info.imageView = targetImageImpl->GetImageView();

            colorAttachments.PushBack(Move(info));
        }

        VkRenderingAttachmentInfo depthAttachment{};
        if (targetImageDepth != nullptr)
        {
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachment.clearValue = { {{1.0f, 0}} };

            ImageView::Impl* targetImageImpl = ImageView::Impl::GetImplementation(targetImageDepth);
            depthAttachment.imageView = targetImageImpl->GetImageView();
        }

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.Size();
        renderingInfo.pColorAttachments = colorAttachments.Data();
        renderingInfo.pDepthAttachment = targetImageDepth == nullptr ? nullptr : &depthAttachment;
        renderingInfo.pStencilAttachment = nullptr;
        renderingInfo.layerCount = 1;
        renderingInfo.renderArea = { {0, 0}, { renderSize.x, renderSize.y } };

	    VulkanHelper::FunctionLoader::vkCmdBeginRendering(commandBufferHandle, &renderingInfo);
    }

    void Renderer::Impl::EndRendering(CommandBuffer& commandBuffer)
    {
        VkCommandBuffer commandBufferHandle = CommandBuffer::Impl::GetImplementation(&commandBuffer)->GetCommandBuffer();
	    VulkanHelper::FunctionLoader::vkCmdEndRendering(commandBufferHandle);
    }

    //
    //  Forward Functions
    //

    VulkanHelper::Expected<Renderer, VHResult> Renderer::New(const Config& config)
    {
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Renderer{ VulkanHelper::Move(implResult.Value()) };
    }

    Renderer::Renderer(Renderer&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Renderer& Renderer::operator=(Renderer&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Renderer(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Renderer::~Renderer()
    {

    }

    Renderer::Renderer(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {
        
    }

    Expected<CommandBuffer*, VHResult> Renderer::BeginFrame()
    {
        return m_Impl->BeginFrame();
    }

    VHResult Renderer::EndFrame()
    {
        return m_Impl->EndFrame();
    }

    void Renderer::BeginRendering(
        CommandBuffer& commandBuffer,
        const VulkanHelper::Vector<ImageView*>& targetImagesColor,
        const ImageView* targetImageDepth,
        glm::uvec2 scissorsStart,
        glm::uvec2 scissorsEnd
    )
    {
        m_Impl->BeginRendering(commandBuffer, targetImagesColor, targetImageDepth, scissorsStart, scissorsEnd);
    }

    void Renderer::EndRendering(CommandBuffer& commandBuffer)
    {
        m_Impl->EndRendering(commandBuffer);
    }

    Image* Renderer::GetCurrentSwapchainImage() const
    {
        return m_Impl->GetCurrentSwapchainImage();
    }

    ImageView* Renderer::GetCurrentSwapchainImageView() const
    {
        return m_Impl->GetCurrentSwapchainImageView();
    }

    Format Renderer::GetSwapchainImageFormat() const
    {
        return m_Impl->GetSwapchainImageFormat();
    }
}