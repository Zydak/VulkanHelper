#include "Window/Window.h"
#include "Vulkan/Instance.h"
#include "Log/Log.h"

int main()
{
    VulkanHelper::Instance instance = VulkanHelper::Instance::New({}).value();
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

    VulkanHelper::Window window = VulkanHelper::Window::New({600, 600, "Example Project", "", true}).value();

    while (!window.WantsToClose())
    {
        VulkanHelper::Window::PollEvents();
    }

    return 0;
}