#include "Core/Error.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Swapchain.h"
#include "Window/Window.h"
#include "Vulkan/Instance.h"
#include "Log/Log.h"
#include "Vulkan/Device.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/Image.h"

int main()
{
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({true}).Value();
    auto physicalDevices = instance.GetSuitablePhysicalDevices({});
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

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", true}).Value();

    if (!window.IsPhysicalDeviceCompatible(*selectedDevice))
    {
        VH_LOG_FATAL("Selected physical device is not compatible with the window surface!");
        return -1;
    }

    VulkanHelper::Device device = VulkanHelper::Device::New({*selectedDevice, &window}).Value();
    VulkanHelper::Swapchain swapchain = VulkanHelper::Swapchain::New({
        .Device = &device,
        .Window = &window,
        .MaxFramesInFlight = 1
    }).Value();

    VulkanHelper::CommandPool pool = VulkanHelper::CommandPool::New({.Device = &device, .QueueFamilyIndex = device.GetQueueFamilyIndices().GraphicsFamily}).Value();
    VulkanHelper::CommandBuffer buffer = pool.AllocateCommandBuffer({}).Value();
    
    while (!window.WantsToClose())
    {
        VulkanHelper::Window::PollEvents();
        VH_ASSERT(swapchain.AcquireNextImage() == VulkanHelper::VHResult::OK, "Couldn't Acquire new image");

        VH_ASSERT(buffer.Begin(VulkanHelper::CommandBuffer::Usage::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Couldn't start recording");

        swapchain.GetCurrentSwapchainImage()->TransitionImageLayout(VulkanHelper::Image::Layout::PRESENT_SRC_KHR, buffer, 0, 1);

        VH_ASSERT(buffer.End() == VulkanHelper::VHResult::OK, "Couldn't end recording");

        VH_ASSERT(swapchain.Submit(buffer) == VulkanHelper::VHResult::OK, "Couldn't Submit to swapchain");
    }

    device.WaitUntilIdle();

    return 0;
}