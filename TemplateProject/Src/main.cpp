#include "Window/Window.h"

int main()
{
	VulkanHelper::Window::CreateInfo createInfo{};
	createInfo.Width = 800;
	createInfo.Height = 600;
	createInfo.Name = "Vulkan Window";

	VulkanHelper::Window window(createInfo);

	while (!window.WantsToClose())
	{
		window.PollEvents();
	}
}