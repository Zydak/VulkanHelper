#include "Core/Error.h"
#include "Window/Window.h"
#include "Vulkan/Instance.h"
#include "Log/Log.h"
#include "Vulkan/Device.h"
#include "Vulkan/Image.h"
#include "Renderer/Renderer.h"

#include "Vulkan/Shader.h"

#include <filesystem>

int main()
{
    VH_LOG_INFO("Current working directory: {}", std::filesystem::current_path().c_str());
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({true}).Value();
    VulkanHelper::Vector<const char*> extensions;
    auto physicalDevices = instance.GetSuitablePhysicalDevices();
    if (physicalDevices.Empty())
    {
        VH_LOG_FATAL("No suitable physical devices found!");
        return -1;
    }
    for (size_t i = 0; i < physicalDevices.Size(); i++)
    {
        VH_LOG_INFO("Found Physical Device: {} (Vendor: {}, Discrete: {})", physicalDevices[i].GetName(), int(physicalDevices[i].GetVendor()), physicalDevices[i].IsDiscrete());
    }

    // Pick discrete GPU if available
    VulkanHelper::PhysicalDevice* selectedDevice = nullptr;
    for (size_t i = 0; i < physicalDevices.Size(); i++)
    {   
        if (physicalDevices[i].IsDiscrete())
        {
            VH_LOG_INFO("Selected Discrete GPU: {}", physicalDevices[i].GetName());
            selectedDevice = &physicalDevices[i];
            break;
        }
    }
    if (selectedDevice == nullptr)
    {
        VH_LOG_WARN("No discrete GPU found, using first available device: {}", physicalDevices[0].GetName());
        selectedDevice = &physicalDevices[0];
    }

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", false}).Value();

    VulkanHelper::Vector<VulkanHelper::Window*> windows;
    windows.PushBack(&window);

    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, std::move(windows)}).Value();

    VulkanHelper::Renderer renderer = VulkanHelper::Renderer::New({&device, &window, 1}).Value();

    VulkanHelper::Shader::InitializeSession("ExampleProject/Shaders/");

    (void)VulkanHelper::Shader::New({&device, "TriangleVertex.slang"});
    (void)VulkanHelper::Shader::New({&device, "TriangleFragment.slang"});
    
    while (!window.WantsToClose())
    {
        VulkanHelper::Window::PollEvents();
        VulkanHelper::CommandBuffer* commandBuffer = renderer.BeginFrame().Value();
        (void)commandBuffer;

        VulkanHelper::Vector<VulkanHelper::ImageView*> views;
        views.PushBack(renderer.GetCurrentSwapchainImageView());
        renderer.BeginRendering(*commandBuffer, views, nullptr);

        renderer.EndRendering(*commandBuffer);

        VH_ASSERT(renderer.EndFrame() == VulkanHelper::VHResult::OK, "Failed to end frame");
    }

    device.WaitUntilIdle();

    return 0;
}