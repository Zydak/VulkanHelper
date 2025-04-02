#include "pch.h"
#include "Entity.h"

namespace VulkanHelper
{

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_Handle(handle), m_Scene(scene)
	{

	}

}