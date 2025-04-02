#pragma once
#include "pch.h"

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "gtc/matrix_transform.hpp"

#include "Quaternion.h"

namespace VulkanHelper
{
	class Transform
	{
	public:
		Transform() = default;
		Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale);

		~Transform() = default;
		Transform(const Transform& other);
		Transform(Transform&& other) noexcept;
		Transform& operator=(const Transform& other);
		Transform& operator=(Transform&& other) noexcept;

		VkTransformMatrixKHR GetKhrMat();
		glm::mat4 GetMat4();

		inline glm::vec3 GetTranslation() const { return m_Translation; }
		inline Quaternion GetRotation() const { return m_Rotation; }
		inline glm::vec3 GetScale() const { return m_Scale; }

		void SetTranslation(const glm::vec3& vec);
		void SetRotation(const glm::vec3& vec);
		void SetRotation(const glm::quat& quat);
		void SetScale(const glm::vec3& vec);

		void AddTranslation(const glm::vec3& vec);
		void AddRotation(const glm::vec3& vec);
		void AddScale(const glm::vec3& vec);

	private:
		glm::vec3 m_Translation{};
		glm::vec3 m_Scale{ 1.0f, 1.0f, 1.0f };
		Quaternion m_Rotation{};
	};

}

