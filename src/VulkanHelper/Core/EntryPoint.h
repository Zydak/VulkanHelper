#pragma once

#include "Application.h"
#include "Vulkan/Device.h"
#include "Vulkan/Window.h"

extern VulkanHelper::Application* VulkanHelper::CreateApplication();

int main()
{
 	auto app = VulkanHelper::CreateApplication();
 	app->Run();

    delete app;

    return 0;
}