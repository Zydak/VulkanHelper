#pragma once
#include "entt.hpp"
#include "pch.h"
#include "Math/Transform.h"

#include "System.h"

namespace VulkanHelper
{
	// forward declaration
	class Entity;
	class AssetManager;

	class Scene
	{
	public:
		void Init();
		void Destroy();

		Scene();
		~Scene();

		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;
		Scene(Scene&& other) noexcept;
		Scene& operator=(Scene&& other) noexcept;

		Entity CreateEntity();
		void DestroyEntity(Entity& entity);

		void InitScripts();
		void DestroyScripts();
		void UpdateScripts(double deltaTime);

		template<typename T>
		void AddSystem()
		{
			m_Systems.emplace_back(new T());
		}

		void InitSystems();
		void DestroySystems();
		void UpdateSystems(double deltaTime);

		inline entt::registry& GetRegistry() { return *m_Registry; }

		inline bool IsInitialized() const { return m_Initialized; }

	private:
		std::shared_ptr<entt::registry> m_Registry = nullptr;
		std::vector<SystemInterface*> m_Systems;

		bool m_Initialized = false;

		void Reset();
	};

}