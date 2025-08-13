#include "Window/Window.h"
#include "WindowImpl.h"

#include "Core/Error.h"
#include "Log/Log.h"

#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

#include "../Vulkan/PhysicalDeviceImpl.h"
#include "../Vulkan/InstanceImpl.h"

namespace VulkanHelper
{
    void Window::Impl::WindowSizeCallback(GLFWwindow* window, int width, int height)
    {
        VulkanHelper::Window::Impl* impl = static_cast<VulkanHelper::Window::Impl*>(glfwGetWindowUserPointer(window));
        impl->m_Width = static_cast<uint32_t>(width);
        impl->m_Height = static_cast<uint32_t>(height);
    }

    Expected<SharedPtr<Window::Impl>, VHResult> Window::Impl::New(
        const SharedPtr<Instance::Impl>& instance,
        uint32_t width,
        uint32_t height,
        const char* name,
        const char* iconPath,
        bool resizable
    )
    {
        // TODO: Icon, for now there's no image loading
        (void)iconPath; // Suppress unused parameter warning
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable);

        std::string nameStr = std::string(name);

        GLFWwindow* window = glfwCreateWindow((int)width, (int)height, nameStr.c_str(), NULL, NULL);
        if (window == nullptr)
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult res = glfwCreateWindowSurface(instance->GetInstance(), window, nullptr, &surface);
        if (res != VK_SUCCESS)
            return VulkanHelper::Unexpected(VHResult(res)); // VkResult maps to VHError so this is legal

        glfwSetWindowSizeCallback(window, WindowSizeCallback);

        return SharedPtr<Impl>(new Impl(instance, window, surface, VulkanHelper::Move(nameStr), width, height));
    }

    Window::Impl::Impl(Impl&& other) noexcept
        : m_Instance(other.m_Instance),
        m_Window(other.m_Window),
        m_Surface(other.m_Surface),
        m_Name(VulkanHelper::Move(other.m_Name)),
        m_Width(other.m_Width),
        m_Height(other.m_Height)
    {
        other.m_Instance = nullptr;
        other.m_Window = nullptr;
        other.m_Surface = VK_NULL_HANDLE;
    }

    Window::Impl& Window::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Instance = other.m_Instance;
        other.m_Instance = nullptr;
        m_Window = other.m_Window;
        other.m_Window = nullptr;
        m_Surface = other.m_Surface;
        other.m_Surface = VK_NULL_HANDLE;
        m_Name = VulkanHelper::Move(other.m_Name);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        
        return *this;
    }

    Window::Impl::~Impl()
    {
        if (m_Surface != VK_NULL_HANDLE)
        {
            VH_LOG_INFO("Destroying Vulkan Surface");
            vkDestroySurfaceKHR(m_Instance->GetInstance(), m_Surface, nullptr);
        }
        if (m_Window != nullptr)
        {
            VH_LOG_INFO("Destroying Window Implementation");
            glfwDestroyWindow(m_Window);
        }
    }

    void Window::Impl::PollEvents()
    {
        glfwPollEvents();
    }

    bool Window::Impl::WantsToClose() const
    {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::Impl::Close()
    {
        glfwSetWindowShouldClose(m_Window, true);
    }

    bool Window::Impl::IsPhysicalDeviceCompatible(const PhysicalDevice& device) const
    {
        SharedPtr<PhysicalDevice::Impl> physicalDeviceImpl = PhysicalDevice::Impl::GetImplementation(device);
        // Check queue family support for presentation
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceImpl->GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return false;
        }
        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDeviceImpl->GetDevice(), &queueFamilyCount, queueFamilies.Data());
        bool supportsPresentation = false;
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (glfwGetPhysicalDevicePresentationSupport(m_Instance->GetInstance(), physicalDeviceImpl->GetDevice(), i) == GLFW_TRUE)
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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDeviceImpl->GetDevice(), m_Surface, &surfaceCapabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDeviceImpl->GetDevice(), m_Surface, &formatCount, nullptr);
        if (formatCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any surface formats!");
            return false;
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDeviceImpl->GetDevice(), m_Surface, &presentModeCount, nullptr);
        if (presentModeCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any present modes!");
            return false;
        }

        // If we reach here, the device is capabale of presenting to a surface in some capacity
        VH_LOG_DEBUG("Physical device {} is compatible with the window surface.", device.GetName());
        return true;
    }

    //
    //  Forward functions
    //

    VulkanHelper::Expected<Window, VHResult> Window::New(const Config& config)
    {
        VH_LOG_INFO("Creating Window Implementation");

        auto implResult = Impl::New(
            Instance::Impl::GetImplementation(config.Instance),
            config.Width,
            config.Height,
            config.Name,
            config.IconPath,
            config.Resizable
        );

        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Window::Impl::CreatePublicInterface(VulkanHelper::Move(implResult.Value()));
    }

    Window::Window(Window&& other) noexcept
        : m_Impl(VulkanHelper::Move(other.m_Impl))
    {}

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Window(); // Clean up current state

        m_Impl = VulkanHelper::Move(other.m_Impl);

        return *this;
    }

    Window::~Window()
    {

    }

    Window::Window(const Window& other)
        : m_Impl(other.m_Impl)
    {
    }

    Window& Window::operator=(const Window& other)
    {
        if (this == &other)
            return *this;

        this->~Window(); // Clean up current state

        m_Impl = other.m_Impl;

        return *this;
    }

    Window::Window()
        : m_Impl(nullptr)
    {
    }

    Window::Window(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {

    }

    const char* Window::GetName() const
    {
        return m_Impl->GetName();
    }

    uint32_t Window::GetWidth() const
    {
        return m_Impl->GetWidth();
    }

    uint32_t Window::GetHeight() const
    {
        return m_Impl->GetHeight();
    }

    bool Window::IsPhysicalDeviceCompatible(const PhysicalDevice& device) const
    {
        return m_Impl->IsPhysicalDeviceCompatible(device);
    }

    bool Window::WantsToClose() const
    {
        return m_Impl->WantsToClose();
    }

    void Window::Close()
    {
        m_Impl->Close();
    }

    void Window::PollEvents()
    {
        Impl::PollEvents();
    }
}