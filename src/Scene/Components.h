#pragma once
#include "pch.h"

#include "Entity.h"
#include "Math/Transform.h"

namespace VulkanHelper
{
	class ScriptInterface
	{
	public:
		ScriptInterface() = default;
		virtual ~ScriptInterface() {}
		virtual void OnCreate() = 0;
		virtual void OnDestroy() = 0;
		virtual void OnUpdate(double deltaTime) = 0;

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		Entity m_Entity; // Assigned when InitScripts is called on the scene object
	};

	class ScriptComponent
	{
	public:
		ScriptComponent() = default;

		~ScriptComponent();

		ScriptComponent(ScriptComponent&& other) noexcept;

		// TODO delete vector from here
		std::vector<ScriptInterface*> Scripts;
		std::vector<std::string> ScriptClassesNames;

		template<typename T>
		void AddScript()
		{
			Scripts.push_back(new T());

			std::string name = typeid(T).name();
			if (name.find("class ") != std::string::npos)
				name = name.substr(6, name.size() - 6);

			ScriptClassesNames.push_back(name);
		}

		void InitializeScripts();
		void UpdateScripts(double deltaTime);
		void DestroyScripts();

		inline std::vector<ScriptInterface*> GetScripts() const { return Scripts; }

		/**
		 * @brief Retrieves script at specified index.
		 *
		 * @return T* if specified T exists at scriptIndex, otherwise nullptr.
		 */
		template<typename T>
		T* GetScript(uint32_t scriptIndex)
		{
			T* returnVal;
			try
			{
				returnVal = dynamic_cast<T*>(Scripts.at(scriptIndex));
			}
			catch (const std::exception&)
			{
				returnVal = nullptr;
			}

			return returnVal;
		}

		inline uint32_t GetScriptCount() const { return (uint32_t)Scripts.size(); }

		std::vector<char> Serialize();
		void Deserialize(const std::vector<char>& bytes);
	};

	class TransformComponent
	{
	public:
		TransformComponent() = default;
		~TransformComponent() = default;
		TransformComponent(TransformComponent&& other) noexcept { Transform = std::move(other.Transform); };
		TransformComponent(const TransformComponent& other) { Transform = other.Transform; };
		TransformComponent& operator=(const TransformComponent& other) { Transform = other.Transform; return *this; };
		TransformComponent& operator=(TransformComponent&& other) noexcept { Transform = std::move(other.Transform); return *this; };

		std::vector<char> Serialize();
		void Deserialize(const std::vector<char>& bytes);

		VulkanHelper::Transform Transform;
	};

	class NameComponent
	{
	public:
		NameComponent() = default;
		~NameComponent() = default;
		NameComponent(NameComponent&& other) noexcept { Name = std::move(other.Name); };
		NameComponent(const NameComponent& other) { Name = other.Name; };
		NameComponent& operator=(const NameComponent& other) { Name = other.Name; return *this; };
		NameComponent& operator=(NameComponent&& other) noexcept { Name = std::move(other.Name); return *this; };

		std::vector<char> Serialize();
		void Deserialize(const std::vector<char>& bytes);

		std::string Name;
	};
}