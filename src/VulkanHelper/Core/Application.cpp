#include "pch.h"
#include "Application.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetManager.h"
#include "Input.h"
#include "Vulkan/DeleteQueue.h"

#include "Scene/Components.h"
#include "Asset/Serializer.h"
#include "Audio/Audio.h"

namespace VulkanHelper
{

	std::shared_ptr<VulkanHelper::Window> InitWindow(const WindowInfo& windowInfo)
	{
		if (!windowInfo.WorkingDirectory.empty())
			std::filesystem::current_path(windowInfo.WorkingDirectory);

		Logger::Init();
		VK_CORE_TRACE("LOGGER INITIALIZED");
		Audio::Init();
		VK_CORE_TRACE("AUDIO INITIALIZED");

		Window::CreateInfo winInfo;
		winInfo.Width = (int)windowInfo.WindowWidth;
		winInfo.Height = (int)windowInfo.WindowHeight;
		winInfo.Name = windowInfo.Name;
		winInfo.Icon = windowInfo.Icon;
		return std::make_shared<Window>(winInfo);
	}

	std::vector<VulkanHelper::Device::PhysicalDevice> QueryDevices(const QueryDevicesInfo& queryInfo)
	{
		Device::CreateInfo deviceInfo{};
		deviceInfo.DeviceExtensions = queryInfo.DeviceExtensions;

		// If the swapchain extension is absent push it
		if (std::find(queryInfo.DeviceExtensions.begin(), queryInfo.DeviceExtensions.end(), VK_KHR_SWAPCHAIN_EXTENSION_NAME) == queryInfo.DeviceExtensions.end())
			deviceInfo.DeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		deviceInfo.OptionalExtensions = queryInfo.OptionalExtensions;
		deviceInfo.Features = queryInfo.Features;
		deviceInfo.Window = queryInfo.Window.get();
		deviceInfo.UseRayTracing = queryInfo.EnableRayTracingSupport;
		deviceInfo.UseMemoryAddress = queryInfo.UseMemoryAddress;
		deviceInfo.IgnoredMessageIDs = queryInfo.IgnoredMessageIDs;

		return Device::Init(deviceInfo);
	}

	void Init(const InitializationInfo& initInfo)
	{
		Device::Init(initInfo.PhysicalDevice);

		Renderer::Init(*initInfo.Window, initInfo.MaxFramesInFlight);
		Input::Init(initInfo.Window->GetGLFWwindow());

		const uint32_t coresCount = std::thread::hardware_concurrency();
		AssetManager::Init({ coresCount / 2 });
		DeleteQueue::Init({ initInfo.MaxFramesInFlight });

		REGISTER_CLASS_IN_SERIALIZER(ScriptComponent);
		REGISTER_CLASS_IN_SERIALIZER(TransformComponent);
		REGISTER_CLASS_IN_SERIALIZER(NameComponent);
		REGISTER_CLASS_IN_SERIALIZER(MeshComponent);
		REGISTER_CLASS_IN_SERIALIZER(MaterialComponent);
		REGISTER_CLASS_IN_SERIALIZER(TonemapperSettingsComponent);
		REGISTER_CLASS_IN_SERIALIZER(BloomSettingsComponent);
	}

	void EndFrame()
	{
		DeleteQueue::UpdateQueue();
	}

	void Destroy()
	{
		vkDeviceWaitIdle(Device::GetDevice());

		Audio::Destroy();
		Renderer::Destroy();
		AssetManager::Destroy();
		DeleteQueue::Destroy();
		Device::Destroy();
	}

}