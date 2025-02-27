#include "Pch.h"
#include "Window.h"

#include "vulkan/vulkan.h"
#include "Logger/Logger.h"

#include "Vulkan/Instance.h"

void VulkanHelper::Window::Destroy()
{
	if (m_Window == nullptr)
		return;

	m_Renderer.Destroy();

	vkDestroySurfaceKHR(Instance::Get()->GetHandle(), m_Surface, nullptr);

	glfwDestroyWindow(m_Window);
	m_Window = nullptr;
}

void VulkanHelper::Window::InitRenderer(Device* device, uint32_t maxFramesInFlight /*= 2*/)
{
	m_Device = device;

	Renderer::CreateInfo createInfo{};
	createInfo.Device = device;
	createInfo.Window = this;
	createInfo.MaxFramesInFlight = maxFramesInFlight;
	createInfo.PresentMode = VK_PRESENT_MODE_FIFO_KHR;

	m_Renderer.Init(createInfo);
}

VulkanHelper::Window::Window(CreateInfo& createInfo)
{
	m_Width = createInfo.Width;
	m_Height = createInfo.Height;
	m_Name = createInfo.Name;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, createInfo.Resizable);

	m_Window = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);
	glfwSetFramebufferSizeCallback(m_Window, ResizeCallback);

	CreateWindowSurface(Instance::Get()->GetHandle());

	VH_INFO("Window created");
}

void VulkanHelper::Window::PollEvents()
{
	m_UserPointer.Window = this;
	glfwSetWindowUserPointer(m_Window, &m_UserPointer);

	glfwPollEvents();
}

void VulkanHelper::Window::ResizeCallback(GLFWwindow* window, int width, int height)
{
	UserPointer* userPointer = (UserPointer*)(glfwGetWindowUserPointer(window));
	userPointer->Window->m_Width = width;
	userPointer->Window->m_Height = height;
}

void VulkanHelper::Window::CreateWindowSurface(VkInstance instance)
{
	VH_CHECK(glfwCreateWindowSurface(instance, m_Window, nullptr, &m_Surface) == VK_SUCCESS, "Failed to create surface!");
}

VulkanHelper::Window& VulkanHelper::Window::operator=(Window&& other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	Move(std::move(other));

	return *this;
}

VulkanHelper::Window::Window(Window&& other) noexcept
{
	if (this == &other)
		return;

	Destroy();

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
	m_Device = other.m_Device;
	m_Renderer = std::move(other.m_Renderer);
}