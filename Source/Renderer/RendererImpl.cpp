#include "RendererImpl.h"
#include "Core/Error.h"
#include "Log/Log.h"

#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"

#include "../Vulkan/ImageImpl.h"

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
        res = m_CommandBuffers[currentFrameIndex].Begin(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);

        if (res != VHResult::OK)
            return Unexpected(res);
        
        return &m_CommandBuffers[currentFrameIndex];
    }

    VHResult Renderer::Impl::EndFrame()
    {
        const uint32_t currentFrameIndex = m_Swapchain.GetCurrentFrameIndex();

        m_Swapchain.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::PRESENT_SRC_KHR, m_CommandBuffers[currentFrameIndex], 0, 1);
        VHResult res = m_CommandBuffers[currentFrameIndex].End();
        if (res != VHResult::OK)
            return res;

        res = m_Swapchain.Submit(m_CommandBuffers[currentFrameIndex]);
        if (res != VHResult::OK)
            return res;

        return VHResult::OK;
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
}