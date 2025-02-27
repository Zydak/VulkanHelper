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

	// Test pipeline
	{
		VulkanHelper::Shader::CreateInfo shaderCreateInfo{};
		shaderCreateInfo.Device = device.get();
		shaderCreateInfo.Filepath = "Src/FragmentTest.hlsl";
		shaderCreateInfo.Type = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		VulkanHelper::Shader fragmentShader(shaderCreateInfo);
		fragmentShader.Compile();

		shaderCreateInfo.Filepath = "Src/VertexTest.hlsl";
		shaderCreateInfo.Type = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		VulkanHelper::Shader vertexShader(shaderCreateInfo);
		vertexShader.Compile();

		VulkanHelper::Pipeline::GraphicsCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.Device = device.get();
		pipelineCreateInfo.DepthClamp = false;
		pipelineCreateInfo.Width = window->GetWidth();
		pipelineCreateInfo.Height = window->GetHeight();
		pipelineCreateInfo.ColorAttachmentCount = 1;
		pipelineCreateInfo.DepthFormat = VK_FORMAT_D16_UNORM;
		pipelineCreateInfo.ColorFormats = { VK_FORMAT_R32G32B32A32_SFLOAT };
		pipelineCreateInfo.Shaders = { &vertexShader, &fragmentShader };

		VulkanHelper::Pipeline pipeline(pipelineCreateInfo);
	}

	{
		VulkanHelper::Buffer::CreateInfo bufferCreateInfo{};
		bufferCreateInfo.Device = device.get();
		bufferCreateInfo.BufferSize = 33;
		bufferCreateInfo.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bufferCreateInfo.UsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.DedicatedAllocation = true;

		VulkanHelper::Buffer buffer(bufferCreateInfo);

		uint32_t testData = 25;
		buffer.WriteToBuffer(&testData, 4, 4);

		testData = 0;

		buffer.ReadFromBuffer(&testData, 4, 4);

		VH_INFO("Data read from buffer: {0}", testData);

		VulkanHelper::Buffer buf1 = std::move(buffer);
	}

	while (!window->WantsToClose())
	{
		window->PollEvents();

		if (window->GetRenderer()->BeginFrame())
		{
			window->GetRenderer()->EndFrame();
		}
	}

	window = nullptr;
	device = nullptr;
	VulkanHelper::Instance::Destroy();
}