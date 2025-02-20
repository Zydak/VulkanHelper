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

	{
		VulkanHelper::Buffer::CreateInfo bufferCreateInfo{};
		bufferCreateInfo.Device = &device;
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
	}

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