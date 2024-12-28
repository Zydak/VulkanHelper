#include "pch.h"
#include "Input.h"

#include "imgui.h"

#include "Utility/Utility.h"

#include "Window.h"

namespace VulkanHelper
{
	static float scrollValue = 0.0f;

	void Input::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		UserPointer* userPointer = (UserPointer*)(glfwGetWindowUserPointer(window));

		auto iter = userPointer->Input->m_Keys.find(key);
		if (iter != userPointer->Input->m_Keys.end())
		{
			for (auto& func : iter->second)
			{
				// If the function returns true, then the event is consumed and will not be passed to the next callback
				bool consume = func(key, scancode, action, mods);
				if (consume)
					break;
			}
		}
	}

	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		scrollValue += (float)yoffset;

		ImGuiIO* io = &ImGui::GetIO();
		io->MouseWheel += (float)yoffset;
	}

	void Input::Init(GLFWwindow* window)
	{
		m_Window = window;

		glfwSetScrollCallback(window, ScrollCallback);
	}

	bool Input::IsKeyPressed(int keyCode)
	{
		return glfwGetKey(m_Window, keyCode) == GLFW_PRESS;
	}

	bool Input::IsMousePressed(int mouseButton)
	{
		return glfwGetMouseButton(m_Window, mouseButton);
	}

	glm::vec2 Input::GetMousePosition()
	{
		double x, y;
		glfwGetCursorPos(m_Window, &x, &y);
		return glm::vec2(x, y);
	}

	float Input::GetScrollValue()
	{
		return scrollValue;
	}

	void Input::SetScrollValue(float x)
	{
		scrollValue = x;
	}

	void Input::SetKeyCallback(int key, std::function<bool(int, int, int, int)> function)
	{
		m_Keys[key].push_back(function);
	}

}