#include "Window/Window.h"

int main()
{
    VulkanHelper::Window window = VulkanHelper::Window::New({600, 600, "Example Project", "", true}).value();

    while (!window.WantsToClose())
    {
        VulkanHelper::Window::PollEvents();
    }

    return 0;
}