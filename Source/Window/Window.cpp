#include "Window/Window.h"

#include "Core/Error.h"
#include "GLFW/glfw3.h"
#include "Log/Log.h"

namespace VulkanHelper
{
    static void ErrorCallback(int errorCode, const char* message)
    {
        VH_LOG_ERROR("GLFW ERROR: Error code: {} | Message: {}", errorCode, message);
    }

    std::expected<Window, VHError> Window::New(const Config& config)
    {
        VH_LOG_INFO("Initializing Window");
        glfwSetErrorCallback(ErrorCallback);

        static bool GLFWInitialized = false;
        if (GLFWInitialized == false)
        {
            if (glfwInit() != GLFW_TRUE)
                return std::unexpected(VHError::FAIL);

            GLFWInitialized = true;
        }

        // TODO: Icon, for now there's no image loading
        glfwWindowHint(GLFW_RESIZABLE, config.Resizable);

        GLFWwindow* window = glfwCreateWindow(config.Width, config.Height, config.Name.c_str(), NULL, NULL);
        if (window == nullptr)
            return std::unexpected(VHError::FAIL);

        glfwMakeContextCurrent(window);

        std::string name = config.Name;
        return Window(window, std::move(name), config.Width, config.Height);
    }

    Window::Window(Window&& other) noexcept
        : m_Window(other.m_Window), m_Name(std::move(other.m_Name)), m_Width(other.m_Width), m_Height(other.m_Height)
    {
        other.m_Window = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this == &other)
            return *this;

        m_Window = other.m_Window;
        other.m_Window = nullptr;
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
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    bool Window::WantsToClose()
    {
        return glfwWindowShouldClose(m_Window);
    }

    void Window::Close()
    {
        glfwSetWindowShouldClose(m_Window, true);
    }
}