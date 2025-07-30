#include "Core/Error.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/Swapchain.h"
#include "Window/Window.h"
#include "Vulkan/Instance.h"
#include "Log/Log.h"
#include "Vulkan/Device.h"
#include "Vulkan/CommandPool.h"

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
        (void)swapchain.AcquireNextImage();

        VH_ASSERT(buffer.Begin(VulkanHelper::CommandBuffer::UsageFlags::ONE_TIME_SUBMIT_BIT) == VulkanHelper::VHResult::OK, "Couldn't start recording");

        // Record

        VH_ASSERT(buffer.End() == VulkanHelper::VHResult::OK, "Couldn't end recording");

        (void)swapchain.Submit(buffer);
    }

    return 0;
}