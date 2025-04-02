#include "pch.h"
#include "Components.h"
#include "Renderer/Renderer.h"

#include "Logger/Logger.h"
#include "Utility/Bytes.h"

#include "Asset/Serializer.h"

namespace VulkanHelper
{
	void ScriptComponent::InitializeScripts()
	{
		for (auto& script : Scripts)
		{
			script->OnCreate();
		}
	}

	void ScriptComponent::UpdateScripts(double deltaTime)
	{
		for (auto& script : Scripts)
		{
			script->OnUpdate(deltaTime);
		}
	}

	void ScriptComponent::DestroyScripts()
	{
		for (auto& script : Scripts)
		{
			script->OnDestroy();
		}
	}

	std::vector<char> ScriptComponent::Serialize()
	{
		std::vector<char> bytes;

		for (int i = 0; i < ScriptClassesNames.size(); i++)
		{
			for (int j = 0; j < ScriptClassesNames[i].size(); j++)
			{
				bytes.push_back(ScriptClassesNames[i][j]);
			}
		}

		bytes.push_back('\0');

		return bytes;
	}

	void ScriptComponent::Deserialize(const std::vector<char>& bytes)
	{
		std::vector<std::string> scriptClassNames(1);

		if (bytes[0] == '\0') // No scripts attached
			return;

		for (int i = 0; i < bytes.size(); i++)
		{
			if (bytes[i] == '\0')
			{
				scriptClassNames.emplace_back("");
			}

			scriptClassNames[scriptClassNames.size() - 1].push_back(bytes[i]);
		}

		scriptClassNames.erase(scriptClassNames.end() - 1); // last index is always empty due to for loop above

		ScriptClassesNames = scriptClassNames;

		for (int i = 0; i < scriptClassNames.size(); i++)
		{
			ScriptInterface* scInterface = (ScriptInterface*)Serializer::CreateRegisteredClass(scriptClassNames[i]);

			VH_ASSERT(scInterface != nullptr, "Script doesn't inherit from script interface!");

			Scripts.push_back(scInterface);
		}
	}

	ScriptComponent::~ScriptComponent()
	{
		for (auto& script : Scripts)
		{
			delete script;
		}
	}

	ScriptComponent::ScriptComponent(ScriptComponent&& other) noexcept
	{
		Scripts = std::move(other.Scripts);
		ScriptClassesNames = std::move(other.ScriptClassesNames);
	}

	std::vector<char> TransformComponent::Serialize()
	{
		return VulkanHelper::Bytes::ToBytes(this, sizeof(TransformComponent));
	}

	void TransformComponent::Deserialize(const std::vector<char>& bytes)
	{
		TransformComponent comp = VulkanHelper::Bytes::FromBytes<TransformComponent>(bytes);
		memcpy(&Transform, &comp.Transform, sizeof(VulkanHelper::Transform));
	}

	std::vector<char> NameComponent::Serialize()
	{
		std::vector<char> bytes;
		for (int i = 0; i < Name.size(); i++)
		{
			bytes.push_back(Name[i]);
		}
		bytes.push_back('\0');

		return bytes;
	}

	void NameComponent::Deserialize(const std::vector<char>& bytes)
	{
		std::string name;

		int index = 0;
		while (true)
		{
			char ch = bytes[index];
			index++;

			if (ch == '\0')
				break;
			else
				name.push_back(ch);
		}

		Name = std::move(name);
	}

}