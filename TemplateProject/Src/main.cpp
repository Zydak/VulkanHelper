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

	VulkanHelper::Window window(createInfo);

	std::vector<VulkanHelper::Instance::PhysicalDevice> devices = VulkanHelper::Instance::Get()->QuerySuitablePhysicalDevices(window.GetSurface(), { VK_KHR_SWAPCHAIN_EXTENSION_NAME });

	VulkanHelper::Device::CreateInfo deviceCreateInfo{};
	deviceCreateInfo.Extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	deviceCreateInfo.PhysicalDevice = devices[0];
	deviceCreateInfo.Surface = window.GetSurface();

	VulkanHelper::Device device(deviceCreateInfo);

	window.InitRenderer(&device);

	while (!window.WantsToClose())
	{
		window.PollEvents();

		if (window.GetRenderer()->BeginFrame())
		{
			window.GetRenderer()->EndFrame();
		}
	}

	window.Destroy();
	device.Destroy();
	VulkanHelper::Instance::Destroy();
}