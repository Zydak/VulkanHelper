#pragma once
#include "Pch.h"

#include "vulkan.hpp"
#include "GLFW/glfw3.h"

namespace VulkanHelper
{
	class Window
	{
	public:
		struct CreateInfo
		{
			uint32_t Width = 0;
			uint32_t Height = 0;
			std::string Name = "";
			std::string Icon = "";

			bool Resizable = false;
		};

		void Init(CreateInfo& createInfo);
		void Destroy();

		Window() = default;
		Window(CreateInfo& createInfo);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;
		Window(Window&& other) noexcept;
		Window& operator=(Window&& other) noexcept;

		void PollEvents();
		void Close(bool value) { glfwSetWindowShouldClose(m_Window, value); }

		[[nodiscard]] bool WantsToClose() const { return glfwWindowShouldClose(m_Window); }
		[[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }
		[[nodiscard]] uint32_t GetWidth() const { return m_Width; }
		[[nodiscard]] uint32_t GetHeight() const { return m_Height; }
		[[nodiscard]] std::string GetName() const { return m_Name; }
		[[nodiscard]] GLFWwindow* GetWindow() const { return m_Window; }
	private:

		void CreateWindowSurface(VkInstance instance);
		GLFWwindow* m_Window = nullptr;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		std::string m_Name = "";
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;

		bool m_Initialized = false;

		void Move(Window&& other) noexcept;
		void Reset();
	};

} // namespace VulkanHelper
