#include "Pch.h"
#include "Window.h"

#include "vulkan/vulkan.h"

void VulkanHelper::Window::Init(CreateInfo& createInfo)
{
	if (m_Initialized)
		Destroy();

	m_Width = createInfo.Width;
	m_Height = createInfo.Height;
	m_Name = createInfo.Name;

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, createInfo.Resizable);

	m_Window = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);

	m_Initialized = true;
}

void VulkanHelper::Window::Destroy()
{
	if (!m_Initialized)
		return;

	glfwDestroyWindow(m_Window);

	Reset();
}

VulkanHelper::Window::Window(CreateInfo& createInfo)
{
	Init(createInfo);
}

void VulkanHelper::Window::PollEvents()
{
	glfwPollEvents();
}

void VulkanHelper::Window::CreateWindowSurface(VkInstance instance)
{
	if (glfwCreateWindowSurface(instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface!");
	}
}

VulkanHelper::Window& VulkanHelper::Window::operator=(Window&& other) noexcept
{
	Move(std::move(other));

	return *this;
}

VulkanHelper::Window::Window(Window&& other) noexcept
{
	Move(std::move(other));
}

VulkanHelper::Window::~Window()
{
	Destroy();
}

void VulkanHelper::Window::Move(Window&& other) noexcept
{
	m_Window = other.m_Window;
	m_Name = std::move(other.m_Name);
	m_Width = other.m_Width;
	m_Height = other.m_Height;
	m_Surface = other.m_Surface;

	m_Initialized = other.m_Initialized;

	other.Reset();
}

void VulkanHelper::Window::Reset()
{
	m_Window = nullptr;
	m_Name = "";
	m_Width = 0;
	m_Height = 0;
	m_Surface = VK_NULL_HANDLE;

	m_Initialized = false;
}