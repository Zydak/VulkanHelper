#include "RendererImpl.h"
#include "Core/Error.h"
#include "Log/Log.h"

#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Device.h"

#include "Vulkan/SwapchainImpl.h"
#include "Vulkan/ImageViewImpl.h"
#include "Vulkan/CommandBufferImpl.h"
#include "Vulkan/CommandPoolImpl.h"
#include "Vulkan/FunctionLoader.h"

#include "Vulkan/DescriptorPoolImpl.h"
#include "Vulkan/SamplerImpl.h"

#include <vulkan/vulkan.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <array>

void ImGui_ImplVulkan_DestroyDeviceObjects();

static void CheckVulkanResult(VkResult result)
{
    if (result != VK_SUCCESS)
    {
        VH_LOG_ERROR("IMGUI VULKAN ERROR");
    }
}

namespace VulkanHelper
{
    Expected<SharedPtr<Renderer::Impl>, VHResult> Renderer::Impl::New(const SharedPtr<Device::Impl>& device, const SharedPtr<Window::Impl>& window)
    {
        auto swapchainResult = Swapchain::Impl::New(device, window, nullptr);
        if (!swapchainResult.HasValue())
        {
            VH_LOG_ERROR("Couldn't create swapchain!");
            return Unexpected(swapchainResult.Error());
        }

        Swapchain swapchain = Swapchain::Impl::CreatePublicInterface(Move(swapchainResult.Value()));

        Device::QueueFamilyIndices queueIndices = device->GetQueueFamilyIndices();
        auto commandPoolRes = CommandPool::Impl::New(device, CommandPool::Flags::RESET_COMMAND_BUFFER_BIT, queueIndices.GraphicsFamily);
        if (!commandPoolRes.HasValue())
        {
            VH_LOG_ERROR("Couldn't create CommandPool!");
            return Unexpected(commandPoolRes.Error());
        }

        CommandPool commandPool = CommandPool::Impl::CreatePublicInterface(Move(commandPoolRes.Value()));

        Vector<CommandBuffer> commandBuffers;
        for (uint32_t i = 0; i < 2; i++)
        {
            CommandBuffer cmdBuf = commandPool.AllocateCommandBuffer({}).Value();
            commandBuffers.PushBack(Move(cmdBuf)); // Allocate cmdBuffer for each frame
        }

        std::array<DescriptorPool::PoolSize, 2> poolSizes = {
            DescriptorPool::PoolSize{ DescriptorType::UNIFORM_BUFFER, 1000 },
            DescriptorPool::PoolSize{ DescriptorType::COMBINED_IMAGE_SAMPLER, 1000 }
        };

        DescriptorPool::Config poolInfo;
        poolInfo.Device = Device::Impl::CreatePublicInterface(device);
        poolInfo.MaxSets = 1000;
        poolInfo.PoolSizes = poolSizes.data();
        poolInfo.PoolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.PoolFlags = DescriptorPool::Flags::FREE_DESCRIPTOR_SET_BIT;

        auto poolResult = DescriptorPool::New(poolInfo);
        if (!poolResult.HasValue())
        {
            VH_LOG_ERROR("Couldn't create ImGui descriptor pool!");
            return Unexpected(poolResult.Error());
        }

        DescriptorPool imguiPool = poolResult.Value();

        float resWidth, resHeight;
        glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &resWidth, &resHeight);

        float scale = glm::min(resWidth, resHeight);

        //
        // ImGui initialization
        //

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImGui_ImplGlfw_InitForVulkan(window->GetWindow(), true);
        ImGui_ImplVulkan_InitInfo info{};
		info.Instance = device->GetInstance()->GetInstance();
		info.PhysicalDevice = device->GetPhysicalDevice()->GetDevice();
		info.Device = device->GetDevice();
		info.Queue = CommandPool::Impl::GetImplementation(commandPool)->GetQueue();
		info.DescriptorPool = DescriptorPool::Impl::GetImplementation(imguiPool)->GetDescriptorPool();
		info.Subpass = 0;
		info.MinImageCount = 2;
		info.ImageCount = 3;
        info.UseDynamicRendering = true;
        info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
        info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.CheckVkResultFn = CheckVulkanResult;
        VkFormat swapchainFormat = (VkFormat)swapchain.GetSwapchainImageFormat();
        info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchainFormat;
		ImGui_ImplVulkan_Init(&info);

        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleDpi = scale;

        style.Alpha = 1.0f;
        style.DisabledAlpha = 0.5f;
        style.WindowPadding = ImVec2(12.0f, 12.0f);
        style.WindowRounding = 0.0f;
        style.WindowBorderSize = 0.0f;
        style.WindowMinSize = ImVec2(20.0f, 20.0f);
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
        style.WindowMenuButtonPosition = ImGuiDir_Right;
        style.ChildRounding = 0.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupRounding = 0.0f;
        style.PopupBorderSize = 1.0f;
        style.FramePadding = ImVec2(20.0f, 3.400000095367432f);
        style.FrameRounding = 2.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(4.300000190734863f, 5.5f);
        style.ItemInnerSpacing = ImVec2(7.099999904632568f, 1.799999952316284f);
        style.CellPadding = ImVec2(12.10000038146973f, 9.199999809265137f);
        style.IndentSpacing = 0.0f;
        style.ColumnsMinSpacing = 4.900000095367432f;
        style.ScrollbarSize = 11.60000038146973f;
        style.ScrollbarRounding = 15.89999961853027f;
        style.TabRounding = 0.0f;
        style.TabBorderSize = 0.0f;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        style.SelectableTextAlign = ImVec2(0.0f, 0.0f);
        
        style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.09250493347644806f, 0.100297249853611f, 0.1158798336982727f, 1.0f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1120669096708298f, 0.1262156516313553f, 0.1545064449310303f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1821731775999069f, 0.1897992044687271f, 0.1974248886108398f, 1.0f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.1545050293207169f, 0.1545048952102661f, 0.1545064449310303f, 1.0f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.1414651423692703f, 0.1629818230867386f, 0.2060086131095886f, 1.0f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.1072951927781105f, 0.107295036315918f, 0.1072961091995239f, 1.0f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.1293079704046249f, 0.1479243338108063f, 0.1931330561637878f, 1.0f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1459212601184845f, 0.1459220051765442f, 0.1459227204322815f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1249424293637276f, 0.2735691666603088f, 0.5708154439926147f, 1.0f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8841201663017273f, 0.7941429018974304f, 0.5615870356559753f, 1.0f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9570815563201904f, 0.9570719599723816f, 0.9570761322975159f, 1.0f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.9356134533882141f, 0.9356129765510559f, 0.9356223344802856f, 1.0f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.266094446182251f, 0.2890366911888123f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);

        //
        //
        //

        return SharedPtr<Impl>( new Impl(
            device,
            window,
            Move(swapchain),
            Move(commandPool),
            Move(commandBuffers),
            imguiPool,
            false,
            false
        ));
    }

    Renderer::Impl::~Impl()
    {
        ImGui_ImplVulkan_DestroyDeviceObjects();
    }

    Renderer::Impl::Impl(Impl&& other) noexcept
        : m_Device(other.m_Device)
        , m_Window(other.m_Window)
        , m_Swapchain(VulkanHelper::Move(other.m_Swapchain))
        , m_CommandPool(VulkanHelper::Move(other.m_CommandPool))
        , m_CommandBuffers(VulkanHelper::Move(other.m_CommandBuffers))
        , m_ImGuiDescriptorPool(VulkanHelper::Move(other.m_ImGuiDescriptorPool))
        , m_IsFrameStarted(other.m_IsFrameStarted)
        , m_IsRenderingStarted(other.m_IsRenderingStarted)
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
        m_ImGuiDescriptorPool = Move(other.m_ImGuiDescriptorPool);
        m_IsFrameStarted = other.m_IsFrameStarted;
        m_IsRenderingStarted = other.m_IsRenderingStarted;

        return *this;
    }

    Expected<CommandBuffer, VHResult> Renderer::Impl::BeginFrame(bool* outWasSwapchainRecreated)
    {
        m_Device->GetDeleteQueue().Update();
        if (outWasSwapchainRecreated)
            *outWasSwapchainRecreated = false;
            
        if (m_Window->GetWidth() == 0 || m_Window->GetHeight() == 0)
            return Unexpected(VHResult::INVALID_WINDOW_SIZE);

        VHResult res = m_Swapchain.AcquireNextImage();
        if (res == VHResult::OUT_OF_DATE_KHR)
        {
            if (m_Window->GetWidth() == 0 || m_Window->GetHeight() == 0)
                return Unexpected(VHResult::INVALID_WINDOW_SIZE);

            res = RecreateSwapchain();
            if (outWasSwapchainRecreated)
                *outWasSwapchainRecreated = true;
            if (res != VHResult::OK)
                return Unexpected(res);
        }
        else if (res != VHResult::OK)
        {
            return Unexpected(res);
        }

        const uint32_t currentFrameIndex = m_Swapchain.GetCurrentFrameIndex();
        res = m_CommandBuffers[currentFrameIndex].BeginRecording(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT);

        if (res != VHResult::OK)
            return Unexpected(res);
        
        m_IsFrameStarted = true;
        return m_CommandBuffers[currentFrameIndex];
    }

    VHResult Renderer::Impl::EndFrame(bool* outWasSwapchainRecreated)
    {
        if (!m_IsFrameStarted)
            return VHResult::OK; // Skip ending frame if it wasn't started

        m_IsFrameStarted = false;

        if (outWasSwapchainRecreated)
            *outWasSwapchainRecreated = false;
        const uint32_t currentFrameIndex = m_Swapchain.GetCurrentFrameIndex();

        m_Swapchain.GetCurrentSwapchainImage().TransitionImageLayout(VulkanHelper::Image::Layout::PRESENT_SRC_KHR, m_CommandBuffers[currentFrameIndex], 0, 1);
        VHResult res = m_CommandBuffers[currentFrameIndex].EndRecording();
        if (res != VHResult::OK)
            return res;

        res = m_Swapchain.Submit(m_CommandBuffers[currentFrameIndex]);
        if (res == VHResult::OUT_OF_DATE_KHR)
        {
            if (m_Window->GetWidth() == 0 || m_Window->GetHeight() == 0)
                return VHResult::INVALID_WINDOW_SIZE;
            
            res = RecreateSwapchain();
            if (outWasSwapchainRecreated)
                *outWasSwapchainRecreated = true;
            if (res != VHResult::OK)
                return res;
        }
        else if (res != VHResult::OK)
        {
            VH_LOG_DEBUG("Failed to submit command buffer");
            return res;
        }

        return VHResult::OK;
    }

    void Renderer::Impl::BeginRendering(
        const VulkanHelper::Vector<SharedPtr<ImageView::Impl>>& targetImagesColor,
        const SharedPtr<ImageView::Impl>& targetImageDepth,
        glm::vec4 clearColor,
        float clearDepth,
        const SharedPtr<ImageView::Impl>& resolveImageView,
        glm::uvec2 scissorsStart,
        glm::uvec2 scissorsEnd
    )
    {
        auto commandBuffer = CommandBuffer::Impl::GetImplementation(m_CommandBuffers[m_Swapchain.GetCurrentFrameIndex()]);

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

        VH_ASSERT(scissorsStart.x <= scissorsEnd.x && scissorsStart.y <= scissorsEnd.y, "ScissorsStart must be smaller or equal than ScissorsEnd!");

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

        VkCommandBuffer commandBufferHandle = commandBuffer->GetCommandBuffer();

	    vkCmdSetViewport(commandBufferHandle, 0, 1, &viewport);
	    vkCmdSetScissor(commandBufferHandle, 0, 1, &scissor);

        VulkanHelper::Vector<VkRenderingAttachmentInfo> colorAttachments;
        for (size_t i = 0; i < targetImagesColor.Size(); i++)
        {
            VkRenderingAttachmentInfo info{};
            info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            if (clearColor.x < 0.0f && clearColor.y < 0.0f && clearColor.z < 0.0f && clearColor.w < 0.0f)
            {
                info.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }
            else
            {
                info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                info.clearValue = { {{clearColor.x, clearColor.y, clearColor.z, clearColor.w}} };
            }
            info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            info.resolveMode = VK_RESOLVE_MODE_NONE;
            if (resolveImageView != nullptr)
            {
                info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
                info.resolveImageView = resolveImageView->GetImageView();
                info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            SharedPtr<ImageView::Impl> targetImageImpl = targetImagesColor[i];
            info.imageView = targetImageImpl->GetImageView();

            colorAttachments.PushBack(Move(info));
        }

        VkRenderingAttachmentInfo depthAttachment{};
        if (targetImageDepth != nullptr)
        {
            depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            if(clearDepth < 0.0f)
            {
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            }
            else
            {
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.clearValue = { {{clearDepth, 0}} };
            }
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            SharedPtr<ImageView::Impl> targetImageImpl = targetImageDepth;
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
        m_IsRenderingStarted = true;
    }

    void Renderer::Impl::EndRendering()
    {
        if (!m_IsRenderingStarted)
            return;

        VkCommandBuffer commandBufferHandle = CommandBuffer::Impl::GetImplementation(m_CommandBuffers[m_Swapchain.GetCurrentFrameIndex()])->GetCommandBuffer();
	    VulkanHelper::FunctionLoader::vkCmdEndRendering(commandBufferHandle);
        m_IsRenderingStarted = false;
    }

    void Renderer::Impl::BeginImGuiRendering(
        glm::vec4 clearColor
    )
    {
        BeginRendering(
            {ImageView::Impl::GetImplementation(m_Swapchain.GetCurrentSwapchainImageView())},
            nullptr,
            clearColor,
            1.0f,
            nullptr,
            {0, 0},
            {m_Swapchain.GetSwapchainImageWidth(), m_Swapchain.GetSwapchainImageHeight()}
        );

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
    }

    void Renderer::Impl::EndImGuiRendering()
    {
        const uint32_t currentFrameIndex = m_Swapchain.GetCurrentFrameIndex();

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer::Impl::GetImplementation(m_CommandBuffers[currentFrameIndex])->GetCommandBuffer());

        EndRendering();
    }

    VHResult Renderer::Impl::RecreateSwapchain()
    {
        m_Device->WaitUntilIdle();
        Swapchain oldSwapchain = VulkanHelper::Move(m_Swapchain);

        auto swapchainResult = Swapchain::Impl::New(m_Device, m_Window, Swapchain::Impl::GetImplementation(oldSwapchain));
        if (!swapchainResult.HasValue())
        {
            VH_LOG_ERROR("Failed to recreate swapchain");
            return swapchainResult.Error();
        }

        m_Swapchain = Move(Swapchain::Impl::CreatePublicInterface(Move(swapchainResult.Value())));
        return VHResult::OK;
    }

    uint32_t Renderer::Impl::CreateImGuiDescriptorSet(const SharedPtr<ImageView::Impl>& imageView, const SharedPtr<Sampler::Impl>& sampler, const Image::Layout& imageLayout)
    {
        VkDescriptorSet set = ImGui_ImplVulkan_AddTexture(
            sampler->GetSampler(),
            imageView->GetImageView(),
            (VkImageLayout)imageLayout
        );

        static uint32_t descriptorSetIndex = 0;
        s_ImGuiDescriptorSets[descriptorSetIndex] = set;
        descriptorSetIndex++;

        return descriptorSetIndex - 1;
    }

    void Renderer::Impl::RenderImGuiImage(uint32_t index, glm::vec2 size)
    {
        VkDescriptorSet descriptorSet = s_ImGuiDescriptorSets[index];
        ImGui::Image(descriptorSet, {size.x, size.y});
    }

    //
    //  Forward Functions
    //

    VulkanHelper::Expected<Renderer, VHResult> Renderer::New(const Config& config)
    {
        VH_LOG_INFO("Creating Renderer Implementation");

        auto implResult = Impl::New(
            Device::Impl::GetImplementation(config.Device),
            Window::Impl::GetImplementation(config.Window)
        );

        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Renderer::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    Renderer::Renderer()
        : m_Impl(nullptr)
    {
    }

    Renderer::Renderer(const Renderer& other)
        : m_Impl(other.m_Impl)
    {}

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

    Renderer& Renderer::operator=(const Renderer& other)
    {
        if (this == &other)
            return *this;

        this->~Renderer(); // Clean up current state

        m_Impl = other.m_Impl;

        return *this;
    }

    Renderer::~Renderer()
    {

    }

    Renderer::Renderer(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
        
    }

    Expected<CommandBuffer, VHResult> Renderer::BeginFrame(bool* outWasSwapchainRecreated)
    {
        return m_Impl->BeginFrame(outWasSwapchainRecreated);
    }

    VHResult Renderer::EndFrame(bool* outWasSwapchainRecreated)
    {
        return m_Impl->EndFrame(outWasSwapchainRecreated);
    }

    void Renderer::BeginRendering(
        const VulkanHelper::Vector<ImageView>& targetImagesColor,
        const ImageView* targetImageDepth,
        glm::vec4 clearColor,
        float clearDepth,
        const ImageView* resolveImageView,
        glm::uvec2 scissorsStart,
        glm::uvec2 scissorsEnd
    )
    {
        SharedPtr<ImageView::Impl> resolveImageViewImpl = nullptr;
        if (resolveImageView != nullptr)
            resolveImageViewImpl = ImageView::Impl::GetImplementation(*resolveImageView);

        SharedPtr<ImageView::Impl> targetImageDepthImpl = nullptr;
        if (targetImageDepth != nullptr)
            targetImageDepthImpl = ImageView::Impl::GetImplementation(*targetImageDepth);

        Vector<SharedPtr<ImageView::Impl>> targetImagesColorImpl;
        for (const auto& imageView : targetImagesColor)
            targetImagesColorImpl.PushBack(ImageView::Impl::GetImplementation(imageView));

        m_Impl->BeginRendering(
            targetImagesColorImpl,
            targetImageDepthImpl,
            clearColor,
            clearDepth,
            resolveImageViewImpl,
            scissorsStart,
            scissorsEnd
        );
    }

    uint32_t Renderer::CreateImGuiDescriptorSet(
        const ImageView& imageView,
        const Sampler& sampler,
        const Image::Layout& imageLayout
    )
    {
        return Renderer::Impl::CreateImGuiDescriptorSet(
            ImageView::Impl::GetImplementation(imageView),
            Sampler::Impl::GetImplementation(sampler),
            imageLayout
        );
    }

    void Renderer::RenderImGuiImage(uint32_t index, glm::vec2 size)
    {
        Renderer::Impl::RenderImGuiImage(index, size);
    }

    void Renderer::EndRendering()
    {
        m_Impl->EndRendering();
    }

    void Renderer::BeginImGuiRendering(glm::vec4 clearColor)
    {
        m_Impl->BeginImGuiRendering(clearColor);
    }

    void Renderer::EndImGuiRendering()
    {
        m_Impl->EndImGuiRendering();
    }

    Image Renderer::GetCurrentSwapchainImage() const
    {
        return m_Impl->GetCurrentSwapchainImage();
    }

    ImageView Renderer::GetCurrentSwapchainImageView() const
    {
        return m_Impl->GetCurrentSwapchainImageView();
    }

    Format Renderer::GetSwapchainImageFormat() const
    {
        return m_Impl->GetSwapchainImageFormat();
    }

    uint32_t Renderer::GetSwapchainImageWidth() const
    {
        return m_Impl->GetSwapchainImageWidth();
    }

    uint32_t Renderer::GetSwapchainImageHeight() const
    {
        return m_Impl->GetSwapchainImageHeight();
    }
}