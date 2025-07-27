#include "Window/Window.h"
#include "Vulkan/Instance.h"
#include "Log/Log.h"
#include "Vulkan/Device.h"

int main()
{
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({true}).value();
    auto physicalDevices = instance.GetSuitablePhysicalDevices({});
    if (physicalDevices.empty())
    {
        VH_LOG_FATAL("No suitable physical devices found!");
        return -1;
    }
    for (const auto& device : physicalDevices)
    {
        VH_LOG_INFO("Found Physical Device: {} (Vendor: {}, Discrete: {})", device.GetName(), int(device.GetVendor()), device.IsDiscrete());
    }

    // Pick discrete GPU if available
    VulkanHelper::PhysicalDevice* selectedDevice = nullptr;
    for (auto& device : physicalDevices)
    {   
        if (device.IsDiscrete())
        {
            VH_LOG_INFO("Selected Discrete GPU: {}", device.GetName());
            selectedDevice = &device;
            break;
        }
    }
    if (selectedDevice == nullptr)
    {
        VH_LOG_WARN("No discrete GPU found, using first available device: {}", physicalDevices[0].GetName());
        selectedDevice = &physicalDevices[0];
    }

    VulkanHelper::Window window = VulkanHelper::Window::New({&instance, 600, 600, "Example Project", "", true}).value();

    if (!window.IsPhysicalDeviceCompatible(*selectedDevice))
    {
        VH_LOG_FATAL("Selected physical device is not compatible with the window surface!");
        return -1;
    }

    VulkanHelper::Device device = VulkanHelper::Device::New({std::move(*selectedDevice), &window}).value();

    while (!window.WantsToClose())
    {
        VulkanHelper::Window::PollEvents();
    }

    return 0;
}