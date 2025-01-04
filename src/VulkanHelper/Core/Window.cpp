#include "pch.h"
#include "Utility/Utility.h"

#include "Window.h"
#include "stbimage/stb_image.h"
#include "Vulkan/Instance.h"

namespace VulkanHelper
{

	void GLFW_CALLBACK(int errCode, const char* message)
	{
		VK_CORE_ERROR("GLFW ERROR CODE: {0}", errCode);
		VK_CORE_ERROR("{0}", message);
	}

	void Window::Init(CreateInfo& createInfo)
	{
		if (m_Initialized)
			Destroy();

		m_Width = createInfo.Width;
		m_Height = createInfo.Height;
		m_Name = createInfo.Name;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, createInfo.Resizable);

		m_Window = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);

		int left, top, right, bottom;
		glfwGetWindowFrameSize(m_Window, &left, &top, &right, &bottom);

		//m_Height -= top + bottom;
		//m_Width -= left + right;
		//
		//glfwSetWindowSize(m_Window, m_Width, m_Height);

		glfwSetFramebufferSizeCallback(m_Window, ResizeCallback);
		glfwSetKeyCallback(m_Window, Input::KeyCallback);
		glfwSetErrorCallback(GLFW_CALLBACK);

		GLFWmonitor** monitorwRaw = glfwGetMonitors(&m_MonitorsCount);
		m_Monitors.resize(m_MonitorsCount);
		for (int i = 0; i < m_MonitorsCount; i++)
		{
			m_Monitors[i].MonitorPtr = monitorwRaw[i];
			m_Monitors[i].Name = glfwGetMonitorName(m_Monitors[i].MonitorPtr);
		}

		int width, height, channels;
		if (createInfo.Icon.empty())
		{
			createInfo.Icon = "../VulkanHelper/assets/Icon.png";
		}
		unsigned char* iconData = stbi_load(createInfo.Icon.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (iconData)
		{
			GLFWimage icon;
			icon.width = width;
			icon.height = height;
			icon.pixels = iconData;

			glfwSetWindowIcon(m_Window, 1, &icon);

			stbi_image_free(iconData);
		}

		m_Input.Init(m_Window);

		m_FramesInFlight = createInfo.FramesInFlight;

		CreateWindowSurface();

		m_Initialized = true;
	}

	// Only call this funtion after vulkan device has been initialized
	void Window::InitRenderer()
	{
		m_Renderer.Init({ this }, m_FramesInFlight);
	}

	void Window::Destroy()
	{
		if (!m_Initialized)
			return;

		m_Renderer.Destroy();

		// Destroy rendering surface
		vkDestroySurfaceKHR(Instance::Get()->GetHandle(), m_Surface, nullptr);

		glfwDestroyWindow(m_Window);
		//glfwTerminate();

		Reset();
	}

	/**
	 * @brief Constructs a Window object with the specified window information.
	 * 
	 * @param winInfo - The window information, including width, height, name, and optional icon.
	 */
	Window::Window(CreateInfo& createInfo)
	{
		Init(createInfo);
	}

	Window::~Window()
	{
		Destroy();
	}

	void Window::CreateWindowSurface()
	{
		VK_CORE_RETURN_ASSERT(glfwCreateWindowSurface(Instance::Get()->GetHandle(), m_Window, nullptr, &m_Surface),
			VK_SUCCESS, 
			"failed to create window surface"
		);
	}

	void Window::PollEvents()
	{
		// Reasing pointers in case the object was moved
		m_UserPointer.Input = &m_Input;
		m_UserPointer.Window = this;
		glfwSetWindowUserPointer(m_Window, &m_UserPointer);

		glfwPollEvents();
	}

	void Window::Resize(const glm::vec2& extent)
	{
		m_Resized = true;
		m_Width = (int)extent.x;
		m_Height = (int)extent.y;

		int left, top, right, bottom;
		glfwGetWindowFrameSize(m_Window, &left, &top, &right, &bottom);

		m_Height -= top + bottom;
		m_Width -= left + right;

		glfwSetWindowSize(m_Window, m_Width, m_Height);
	}

	void Window::SetFullscreen(bool val, GLFWmonitor* Monitor)
	{
		if (val)
			glfwSetWindowMonitor(m_Window, Monitor, 0, 0, m_Width, m_Height, GLFW_DONT_CARE);
		else
			glfwSetWindowMonitor(m_Window, NULL, 0, 0, m_Width, m_Height, GLFW_DONT_CARE);
	}

	void Window::ResizeCallback(GLFWwindow* window, int width, int height) {

		UserPointer* userPointer = (UserPointer*)(glfwGetWindowUserPointer(window));
		userPointer->Window->m_Resized = true;
		userPointer->Window->m_Width = width;
		userPointer->Window->m_Height = height;
	}

	void Window::Reset()
	{
		m_Width = 0;
		m_Height = 0;
		m_Name.clear();
		m_Resized = false;
		m_Window = nullptr;
		m_Monitors.clear();
		m_MonitorsCount = 0;
		m_Initialized = false;
		m_FramesInFlight = 0;
	}

}