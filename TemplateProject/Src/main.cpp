#include "VulkanHelper.h"

struct PushData
{
	glm::mat4 MVP;
};

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

	VulkanHelper::PushConstant<PushData> pushConstant({ VK_SHADER_STAGE_VERTEX_BIT });

	std::vector<float> vertices = {
		// Coords			// Color
		 0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, // TOP
		 0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // RIGHT BOTTOM
		-0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // LEFT BOTTOM
	};

	std::vector<uint32_t> indices = { 0, 1, 2 };

	std::vector<VulkanHelper::Mesh::InputAttribute> inputAttributes = { { VK_FORMAT_R32G32B32_SFLOAT, 0 }, { VK_FORMAT_R32G32B32_SFLOAT, 12 } };

	VulkanHelper::Mesh::CreateInfo meshCreateInfo{};
	meshCreateInfo.Device = device.get();
	meshCreateInfo.InputAttributes = inputAttributes;
	meshCreateInfo.VertexData = vertices.data();
	meshCreateInfo.VertexDataSize = vertices.size() * sizeof(float);
	meshCreateInfo.IndexData = indices;
	meshCreateInfo.VertexSize = sizeof(float) * 6;

	VulkanHelper::Mesh mesh;
	(void)mesh.Init(meshCreateInfo);

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
	pipelineCreateInfo.BindingDesc = { mesh.GetBindingDescription() };
	pipelineCreateInfo.AttributeDesc = mesh.GetInputAttributes();
	pipelineCreateInfo.PushConstants = pushConstant.GetRangePtr();

	VulkanHelper::Pipeline pipeline;
	pipeline.Init(pipelineCreateInfo);

	VulkanHelper::Transform transform;
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	glm::mat4 view = glm::mat4{ 1.0f };

	while (!window->WantsToClose())
	{
		window->PollEvents();

		if (window->GetInput()->IsKeyPressed(VH_KEY_SPACE))
			transform.AddScale(glm::vec3(0.1f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_LEFT_ALT))
			transform.AddScale(glm::vec3(-0.1f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_S))
			transform.AddTranslation(glm::vec3(0.0f, 0.1f, 0.0f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_W))
			transform.AddTranslation(glm::vec3(0.0f, -0.1f, 0.0f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_A))
			transform.AddTranslation(glm::vec3(-0.1f, 0.0f, 0.0f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_D))
			transform.AddTranslation(glm::vec3(0.1f, 0.0f, 0.0f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_Q))
			transform.AddTranslation(glm::vec3(0.0f, 0.0f, -0.1f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_E))
			transform.AddTranslation(glm::vec3(0.0f, 0.0f, 0.1f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_F))
			transform.AddRotation(glm::vec3(0.0f, 0.5f, 0.0f));
		if (window->GetInput()->IsKeyPressed(VH_KEY_G))
			transform.AddRotation(glm::vec3(0.0f, -0.5f, 0.0f));

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

			PushData data;
			data.MVP = glm::transpose(proj * view * transform.GetMat4());

			VH_TRACE("{}", transform.GetScale().x);

			pushConstant.SetData(data);
			pushConstant.Push(pipeline.GetPipelineLayout(), window->GetRenderer()->GetCurrentCommandBuffer(), 0);

			mesh.Bind(window->GetRenderer()->GetCurrentCommandBuffer());
			mesh.Draw(window->GetRenderer()->GetCurrentCommandBuffer(), 1);

			window->GetRenderer()->EndRendering();

			window->GetRenderer()->EndFrame();
		}
	}

	window = nullptr;
	device = nullptr;
	VulkanHelper::Instance::Destroy();
}