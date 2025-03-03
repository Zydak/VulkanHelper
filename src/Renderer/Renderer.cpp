#include "Pch.h"
#include "Renderer.h"

#include "Logger/Logger.h"
#include "Window/Window.h"
#include "Vulkan/LoadedFunctions.h"

void VulkanHelper::Renderer::Destroy()
{
	m_Swapchain = nullptr;
	m_CommandBuffers.clear();
}

VulkanHelper::Renderer::Renderer(const CreateInfo& createInfo)
{
	m_Device = createInfo.Device;
	m_Window = createInfo.Window;

	m_PreviousExtent = createInfo.Window->GetExtent();
	m_MaxFramesInFlight = createInfo.MaxFramesInFlight;

	m_Swapchain = std::make_unique<Swapchain>(Swapchain::CreateInfo{ createInfo.Device, createInfo.Window->GetExtent(), createInfo.PresentMode, createInfo.MaxFramesInFlight, createInfo.Window->GetSurface() });

	CreateCommandBuffers();
}

VkCommandBuffer VulkanHelper::Renderer::GetCurrentCommandBuffer()
{
	VH_ASSERT(m_IsFrameStarted, "Cannot get command buffer when frame is not in progress");
	return m_CommandBuffers[m_CurrentImageIndex];
}

bool VulkanHelper::Renderer::BeginFrame()
{
	VH_CHECK(!m_IsFrameStarted, "Can't call BeginFrame while already in progress!");

	auto extent = m_Window->GetExtent();
	if (extent.width == 0 || extent.height == 0)
	{
		// If the extent is 0 the window is either minimized or someone resized it to 0. In either case skip the rendering
		return false;
	}

	bool windowWasResized = extent.width != m_PreviousExtent.width || extent.height != m_PreviousExtent.height;
	if (windowWasResized)
	{
		m_PreviousExtent = extent;
		RecreateSwapchain();

		//return false;
	}

	auto result = m_Swapchain->AcquireNextImage(m_CurrentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		RecreateSwapchain();

		//return false;
	}

	m_IsFrameStarted = true;
	auto commandBuffer = GetCurrentCommandBuffer();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VH_CHECK(
		vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS,
		"failed to begin recording command buffer!"
	);

	return true;
}

void VulkanHelper::Renderer::EndFrame()
{
	auto commandBuffer = GetCurrentCommandBuffer();
	VH_ASSERT(m_IsFrameStarted, "Cannot call EndFrame while frame is not in progress");

	// End recording the command buffer
	auto success = vkEndCommandBuffer(commandBuffer);
	VH_ASSERT(success == VK_SUCCESS, "Failed to record command buffer!");

	if (m_Swapchain->SubmitCommandBuffer(commandBuffer, m_CurrentImageIndex) == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapchain();
		return;
	}

	// End the frame and update frame index
	m_IsFrameStarted = false;
	m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % m_MaxFramesInFlight;
}

void VulkanHelper::Renderer::BeginRendering(const std::vector<VkRenderingAttachmentInfo>& colorAttachments, VkRenderingAttachmentInfo* depthAttachment, const VkExtent2D& renderSize)
{
	VH_ASSERT(m_IsFrameStarted, "Cannot call BeginRendering while frame is not in progress");

	// Set up viewport
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)renderSize.width;
	viewport.height = (float)renderSize.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Set up scissor
	VkRect2D scissor{
		{0, 0},
		renderSize
	};

	// Set viewport and scissor for the current command buffer
	vkCmdSetViewport(GetCurrentCommandBuffer(), 0, 1, &viewport);
	vkCmdSetScissor(GetCurrentCommandBuffer(), 0, 1, &scissor);


	VkRenderingInfo renderingInfo{};
	renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
	renderingInfo.colorAttachmentCount = (uint32_t)colorAttachments.size();
	renderingInfo.pColorAttachments = colorAttachments.data();
	renderingInfo.pDepthAttachment = depthAttachment;
	renderingInfo.pStencilAttachment = nullptr;
	renderingInfo.layerCount = 1;
	renderingInfo.renderArea = { {0, 0}, renderSize };

	VulkanHelper::vkCmdBeginRendering(m_CommandBuffers[m_CurrentImageIndex], &renderingInfo);
}

void VulkanHelper::Renderer::EndRendering()
{
	VH_ASSERT(m_IsFrameStarted, "Cannot call EndRendering while frame is not in progress");

	VulkanHelper::vkCmdEndRendering(m_CommandBuffers[m_CurrentImageIndex]);
}

VulkanHelper::Renderer::~Renderer()
{
	Destroy();
}

VulkanHelper::Renderer& VulkanHelper::Renderer::operator=(Renderer&& other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	Move(std::move(other));

	return *this;
}

VulkanHelper::Renderer::Renderer(Renderer&& other) noexcept
{
	if (this == &other)
		return;

	Destroy();

	Move(std::move(other));
}

void VulkanHelper::Renderer::RecreateSwapchain()
{
	auto extent = m_Window->GetExtent();

	m_Device->WaitUntilIdle();

	std::unique_ptr<Swapchain> oldSwapchain = std::move(m_Swapchain);

	m_Swapchain = std::make_unique<Swapchain>(Swapchain::CreateInfo{ m_Device, m_Window->GetExtent(), oldSwapchain->GetCurrentPresentMode(), m_MaxFramesInFlight, m_Window->GetSurface(), oldSwapchain.get() });
}

void VulkanHelper::Renderer::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_Swapchain->GetImageCount());

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_Device->GetGraphicsCommandPool()->GetHandle();
	allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

	// Allocate primary command buffers
	VH_CHECK(
		vkAllocateCommandBuffers(m_Device->GetHandle(), &allocInfo, m_CommandBuffers.data()) == VK_SUCCESS,
		"Failed to allocate command buffers!"
	);

	for (uint32_t i = 0; i < m_Swapchain->GetImageCount(); i++)
	{
		m_Device->SetObjectName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)m_CommandBuffers[i], "Main Frame Command Buffer");
	}
}

void VulkanHelper::Renderer::Move(Renderer&& other) noexcept
{
	m_Swapchain = std::move(other.m_Swapchain);
	other.m_Swapchain = nullptr;

	m_Device = other.m_Device;
	other.m_Device = nullptr;

	m_Window = other.m_Window;
	other.m_Window = nullptr;

	m_CurrentFrameIndex = other.m_CurrentFrameIndex;
	other.m_CurrentFrameIndex = 0;

	m_CurrentImageIndex = other.m_CurrentImageIndex;
	other.m_CurrentImageIndex = 0;

	m_MaxFramesInFlight = other.m_MaxFramesInFlight;
	other.m_MaxFramesInFlight = 0;

	m_CommandBuffers = std::move(other.m_CommandBuffers);
	other.m_CommandBuffers.clear();

	m_IsFrameStarted = other.m_IsFrameStarted;
	other.m_IsFrameStarted = false;
}