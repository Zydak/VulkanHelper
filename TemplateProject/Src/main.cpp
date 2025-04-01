#include "VulkanHelper.h"

int main()
{
	VulkanHelper::Instance::CreateInfo instanceCreateInfo{};
	VulkanHelper::Instance::Init(instanceCreateInfo);

	VulkanHelper::Window::CreateInfo createInfo{};
	createInfo.Width = 800;
	createInfo.Height = 600;
	createInfo.Name = "Vulkan Window";
	createInfo.Resizable = true;

	std::unique_ptr<VulkanHelper::Window> window = std::make_unique<VulkanHelper::Window>(createInfo);

	std::vector<VulkanHelper::Instance::PhysicalDevice> devices = VulkanHelper::Instance::Get()->QuerySuitablePhysicalDevices(window->GetSurface(), { VK_KHR_SWAPCHAIN_EXTENSION_NAME });

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{};
	dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamicRenderingFeatures.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceFeatures2 features{};
	features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	features.pNext = &dynamicRenderingFeatures;

	VulkanHelper::Device::CreateInfo deviceCreateInfo{};
	deviceCreateInfo.Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };
	deviceCreateInfo.PhysicalDevice = devices[0];
	deviceCreateInfo.Surface = window->GetSurface();
	deviceCreateInfo.Features = features;

	std::unique_ptr<VulkanHelper::Device> device = std::make_unique<VulkanHelper::Device>(deviceCreateInfo);

	window->InitRenderer(device.get());

	VulkanHelper::Shader::CreateInfo shaderCreateInfo{};
	shaderCreateInfo.Device = device.get();
	shaderCreateInfo.Filepath = "Src/FragmentTest.hlsl";
	shaderCreateInfo.Type = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
	VulkanHelper::Shader fragmentShader;
	(void)fragmentShader.Init(shaderCreateInfo);

	shaderCreateInfo.Filepath = "Src/VertexTest.hlsl";
	shaderCreateInfo.Type = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
	VulkanHelper::Shader vertexShader;
	(void)vertexShader.Init(shaderCreateInfo);

	VulkanHelper::Pipeline::GraphicsCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.Device = device.get();
	pipelineCreateInfo.DepthClamp = false;
	pipelineCreateInfo.Width = window->GetWidth();
	pipelineCreateInfo.Height = window->GetHeight();
	pipelineCreateInfo.ColorAttachmentCount = 1;
	pipelineCreateInfo.DepthFormat = VK_FORMAT_D16_UNORM;
	pipelineCreateInfo.ColorFormats = { VK_FORMAT_R8G8B8A8_UNORM };
	pipelineCreateInfo.Shaders = { &vertexShader, &fragmentShader };

	VulkanHelper::Pipeline pipeline;
	pipeline.Init(pipelineCreateInfo);

	while (!window->WantsToClose())
	{
		window->PollEvents();

		if (window->GetRenderer()->BeginFrame())
		{
			VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorAttachment.clearValue = { 0.1f, 0.1f, 0.1f, 1.0f };
			colorAttachment.imageView = window->GetRenderer()->GetSwapchain()->GetPresentableImageView(window->GetRenderer()->GetCurrentImageIndex());

			window->GetRenderer()->BeginRendering({ colorAttachment }, nullptr, window->GetExtent());

			pipeline.Bind(window->GetRenderer()->GetCurrentCommandBuffer());
			vkCmdDraw(window->GetRenderer()->GetCurrentCommandBuffer(), 3, 1, 0, 0);

			window->GetRenderer()->EndRendering();

			window->GetRenderer()->EndFrame();
		}
	}

	window = nullptr;
	device = nullptr;
	VulkanHelper::Instance::Destroy();
}