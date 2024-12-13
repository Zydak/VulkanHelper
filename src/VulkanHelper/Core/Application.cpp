#include "pch.h"
#include "Application.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetManager.h"
#include "Input.h"
#include "Vulkan/DeleteQueue.h"

#include "Scene/Components.h"
#include "Asset/Serializer.h"

namespace VulkanHelper
{
	Application::Application(const ApplicationInfo& appInfo)
		: m_ApplicationInfo(appInfo)
	{
		if (!appInfo.WorkingDirectory.empty())
			std::filesystem::current_path(appInfo.WorkingDirectory);

		Logger::Init();
		VK_CORE_TRACE("LOGGER INITIALIZED");
		Window::CreateInfo winInfo;
		winInfo.Width = (int)appInfo.WindowWidth;
		winInfo.Height = (int)appInfo.WindowHeight;
		winInfo.Name = appInfo.Name;
		winInfo.Icon = appInfo.Icon;
		m_Window = std::make_shared<Window>(winInfo);

		Device::CreateInfo deviceInfo{};
		deviceInfo.DeviceExtensions = appInfo.DeviceExtensions;
		deviceInfo.OptionalExtensions = appInfo.OptionalExtensions;
		deviceInfo.Features = appInfo.Features;
		deviceInfo.Window = m_Window.get();
		deviceInfo.UseRayTracing = appInfo.EnableRayTracingSupport;
		deviceInfo.UseMemoryAddress = appInfo.UseMemoryAddress;
		deviceInfo.IgnoredMessageIDs = std::move(appInfo.IgnoredMessageIDs);

		std::vector<Device::PhysicalDevice> devices = Device::Init(deviceInfo);

		Device::PhysicalDevice finalChoice;
		for (int i = 0; i < devices.size(); i++)
		{
			if (devices[i].IsSuitable())
			{
				if (finalChoice.Discrete == false)
					finalChoice = devices[i];
			}
		}

		Device::Init(finalChoice);

		Renderer::Init(*m_Window, appInfo.MaxFramesInFlight);
		Input::Init(m_Window->GetGLFWwindow());

		const uint32_t coresCount = std::thread::hardware_concurrency();
		AssetManager::Init({ coresCount / 2 });
		DeleteQueue::Init({ appInfo.MaxFramesInFlight });

		REGISTER_CLASS_IN_SERIALIZER(ScriptComponent);
		REGISTER_CLASS_IN_SERIALIZER(TransformComponent);
		REGISTER_CLASS_IN_SERIALIZER(NameComponent);
		REGISTER_CLASS_IN_SERIALIZER(MeshComponent);
		REGISTER_CLASS_IN_SERIALIZER(MaterialComponent);
		REGISTER_CLASS_IN_SERIALIZER(TonemapperSettingsComponent);
		REGISTER_CLASS_IN_SERIALIZER(BloomSettingsComponent);
	}

	Application::~Application()
	{
		VK_CORE_INFO("Closing");
	}

	void Application::Run()
	{
		VK_CORE_TRACE("\n\n\n\nMAIN LOOP START\n\n\n\n");
		Timer timer;
		double deltaTime = 0.0f;

		while (!m_Window->ShouldClose())
		{
			timer.Reset();
			m_Window->PollEvents();

			OnUpdate(deltaTime);
			deltaTime = timer.ElapsedSeconds();

			DeleteQueue::UpdateQueue();
		}

		vkDeviceWaitIdle(Device::GetDevice());

		Renderer::Destroy();
		Destroy();
		AssetManager::Destroy();
		DeleteQueue::Destroy();
		Device::Destroy();
	}
}