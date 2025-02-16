#include "VulkanHelper.h"

int main()
{
	VulkanHelper::Logger::Init();

	VulkanHelper::Window::CreateInfo createInfo{};
	createInfo.Width = 800;
	createInfo.Height = 600;
	createInfo.Name = "Vulkan Window";

	VulkanHelper::Window window(createInfo);
	VH_INFO("Window created");

	while (!window.WantsToClose())
	{
		window.PollEvents();
	}
}