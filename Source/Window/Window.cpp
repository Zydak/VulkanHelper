#include "Window/Window.h"

#include "Core/Error.h"
#include "Log/Log.h"
#include <vulkan/vulkan.h>
#include "GLFW/glfw3.h"

namespace VulkanHelper
{
    class Window::Impl
    {
    public:
        static VulkanHelper::Expected<Impl*, VHResult> New(const Config& config);

        ~Impl();

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        static void PollEvents();
        [[nodiscard]] bool WantsToClose() const;
        void Close();

        [[nodiscard]] bool IsPhysicalDeviceCompatible(const PhysicalDevice& device) const;
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Height; }
        [[nodiscard]] inline std::string GetName() const { return m_Name; }
        [[nodiscard]] inline VkSurfaceKHR GetSurface() const { return m_Surface; }

    private:
        // Constructable only by calling New(...)
        Impl(Instance* instance, GLFWwindow* window, VkSurfaceKHR surface, std::string&& name, uint32_t width, uint32_t height)
            : m_Instance(instance),
            m_Window(window),
            m_Surface(surface),
            m_Name(std::move(name)),
            m_Width(width),
            m_Height(height)
        {}

        Instance*       m_Instance;
        GLFWwindow*     m_Window;
        VkSurfaceKHR    m_Surface;
        std::string     m_Name;
        uint32_t        m_Width;
        uint32_t        m_Height;
    };

    VulkanHelper::Expected<Window::Impl*, VHResult> Window::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Initializing Window");

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

        return new Impl(config.Instance, window, surface, std::move(name), config.Width, config.Height);
    }

    Window::Impl::Impl(Impl&& other) noexcept
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
        m_Name = std::move(other.m_Name);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        
        return *this;
    }

    Window::Impl::~Impl()
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
        vkGetPhysicalDeviceQueueFamilyProperties(device.GetDevice(), &queueFamilyCount, nullptr);
        if (queueFamilyCount == 0)
        {
            VH_LOG_ERROR("Physical device does not support any queue families!");
            return false;
        }
        VulkanHelper::Vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device.GetDevice(), &queueFamilyCount, queueFamilies.Data());
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

        return Window{ implResult.Value() };
    }

    Window::Window(Window&& other) noexcept
        : m_Impl(other.m_Impl)
    {
        other.m_Impl = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Window(); // Clean up current state

        m_Impl = other.m_Impl;
        other.m_Impl = nullptr;

        return *this;
    }

    Window::~Window()
    {
        if (m_Impl != nullptr)
        {
            delete m_Impl;
            m_Impl = nullptr;
            VH_LOG_INFO("Destroying Window");
        }
    }

    std::string Window::GetName() const
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