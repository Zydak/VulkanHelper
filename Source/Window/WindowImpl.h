#pragma once
#include "Window/Window.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "Vulkan/Instance.h"

namespace VulkanHelper
{
    class Window::Impl
    {
    public:
        static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Instance::Impl>& instance,
            uint32_t width,
            uint32_t height,
            const char* name,
            const char* iconPath,
            bool resizable
        );

        ~Impl();

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Window& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Window CreatePublicInterface(const SharedPtr<Impl>& impl) { return Window(impl); }

        static void PollEvents();
        [[nodiscard]] bool WantsToClose() const;
        void Close();

        [[nodiscard]] bool IsPhysicalDeviceCompatible(const PhysicalDevice& device) const;
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Height; }
        [[nodiscard]] inline const char* GetName() const { return m_Name.c_str(); }
        [[nodiscard]] inline VkSurfaceKHR GetSurface() const { return m_Surface; }
        [[nodiscard]] inline GLFWwindow* GetWindow() const { return m_Window; }

    private:
        // Constructable only by calling New(...)
        Impl(const SharedPtr<Instance::Impl>& instance, GLFWwindow* window, VkSurfaceKHR surface, std::string&& name, uint32_t width, uint32_t height)
            :
            m_Instance(instance),
            m_Window(window),
            m_Surface(surface),
            m_Name(VulkanHelper::Move(name)),
            m_Width(width),
            m_Height(height)
        {
            glfwSetWindowUserPointer(m_Window, this);
        }

        static void WindowSizeCallback(GLFWwindow* window, int width, int height);

        SharedPtr<Instance::Impl> m_Instance;
        GLFWwindow*     m_Window;
        VkSurfaceKHR    m_Surface;
        std::string     m_Name;
        uint32_t        m_Width;
        uint32_t        m_Height;
    };
}