#include "Window/Window.h"

#include "Core/Error.h"
#include "Log/Log.h"
#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

namespace VulkanHelper
{
    std::expected<Window, VHError> Window::New(const Config& config)
    {
        VH_LOG_INFO("Initializing Window");

        // TODO: Icon, for now there's no image loading
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, config.Resizable);

        GLFWwindow* window = glfwCreateWindow(config.Width, config.Height, config.Name.c_str(), NULL, NULL);
        if (window == nullptr)
            return std::unexpected(VHError::INITIALIZATION_FAILED);

        std::string name = config.Name;

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult res = glfwCreateWindowSurface(config.Instance->GetInstance(), window, nullptr, &surface);
        if (res != VK_SUCCESS)
            return std::unexpected(VHError(res)); // VkResult maps to VHError so this is legal

        return Window(config.Instance, window, surface, std::move(name), config.Width, config.Height);
    }

    Window::Window(Window&& other) noexcept
        : m_Instance(other.m_Instance),
        m_Window(other.m_Window),
        m_Surface(other.m_Surface),
        m_Name(std::move(other.m_Name)),
        m_Width(other.m_Width),
        m_Height(other.m_Height)
    {
        other.m_Instance = nullptr;
        other.m_Window = nullptr;
        other.m_Surface = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Instance = other.m_Instance;
        other.m_Instance = nullptr;
        m_Window = other.m_Window;
        other.m_Window = nullptr;
        m_Surface = other.m_Surface;
        other.m_Surface = nullptr;
        m_Name = std::move(other.m_Name);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        
        return *this;
    }

    Window::~Window()
    {
        if (m_Window != nullptr)
        {
            glfwDestroyWindow(m_Window);
            VH_LOG_INFO("Destroying Window");
        }
        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance->GetInstance(), m_Surface, nullptr);
            VH_LOG_INFO("Destroying Vulkan Surface");
        }
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    bool Window::WantsToClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::Close()
    {
        glfwSetWindowShouldClose(m_Window, true);
    }

    bool Window::IsPhysicalDeviceCompatible(const PhysicalDevice& device) const
    {
        // Check queue family support for presentation
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device.GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return false;
        }
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device.GetDevice(), &queueFamilyCount, queueFamilies.data());
        bool supportsPresentation = false;
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (glfwGetPhysicalDevicePresentationSupport(m_Instance->GetInstance(), device.GetDevice(), i) == GLFW_TRUE)
            {
                supportsPresentation = true; // At least one queue family supports presentation
                break;
            }
        }
        if (!supportsPresentation)
        {
            VH_LOG_ERROR("Physical device does not support presentation to the window surface!");
            return false;
        }

        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.GetDevice(), m_Surface, &surfaceCapabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.GetDevice(), m_Surface, &formatCount, nullptr);
        if (formatCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any surface formats!");
            return false;
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.GetDevice(), m_Surface, &presentModeCount, nullptr);
        if (presentModeCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any present modes!");
            return false;
        }

        // If we reach here, the device is capabale of presenting to a surface in some capacity
        VH_LOG_DEBUG("Physical device {} is compatible with the window surface.", device.GetName());
        return true;
    }
}