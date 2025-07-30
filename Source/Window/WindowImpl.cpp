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
    VulkanHelper::Expected<VulkanHelper::UniquePtr<Window::Impl>, VHResult> Window::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Creating Window Implementation");

        // TODO: Icon, for now there's no image loading
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, config.Resizable);

        std::string name = std::string(config.Name);

        GLFWwindow* window = glfwCreateWindow(config.Width, config.Height, name.c_str(), NULL, NULL);
        if (window == nullptr)
            return VulkanHelper::Unexpected(VHResult::INITIALIZATION_FAILED);

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkResult res = glfwCreateWindowSurface(config.Instance->GetInstance(), window, nullptr, &surface);
        if (res != VK_SUCCESS)
            return VulkanHelper::Unexpected(VHResult(res)); // VkResult maps to VHError so this is legal

        return VulkanHelper::UniquePtr(new Impl(config.Instance->m_Impl.Get(), window, surface, VulkanHelper::Move(name), config.Width, config.Height));
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
        other.m_Surface = nullptr;
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
        other.m_Surface = nullptr;
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
        // Check queue family support for presentation
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device.m_Impl->GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return false;
        }
        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device.m_Impl->GetDevice(), &queueFamilyCount, queueFamilies.Data());
        bool supportsPresentation = false;
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (glfwGetPhysicalDevicePresentationSupport(m_Instance->GetInstance(), device.m_Impl->GetDevice(), i) == GLFW_TRUE)
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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.m_Impl->GetDevice(), m_Surface, &surfaceCapabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.m_Impl->GetDevice(), m_Surface, &formatCount, nullptr);
        if (formatCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any surface formats!");
            return false;
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.m_Impl->GetDevice(), m_Surface, &presentModeCount, nullptr);
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
        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return VulkanHelper::Unexpected(implResult.Error());
        }

        return Window{ VulkanHelper::Move(implResult.Value()) };
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

    Window::Window(VulkanHelper::UniquePtr<Impl>&& impl)
        : m_Impl(VulkanHelper::Move(impl))
    {

    }

    const char* Window::GetName() const
    {
        return m_Impl->GetName();
    }

    VkSurfaceKHR Window::GetSurface() const
    {
        return m_Impl->GetSurface();
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