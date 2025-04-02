#include "pch.h"
#include "Transform.h"

namespace VulkanHelper
{

	Transform::Transform(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	{
		m_Translation = translation;
		m_Rotation = rotation;
		m_Scale = scale;
	}

	Transform::Transform(const Transform& other)
	{
		m_Rotation = other.m_Rotation;
		m_Scale = other.m_Scale;
		m_Translation = other.m_Translation;
	}

	Transform::Transform(Transform&& other) noexcept
	{
		m_Rotation = std::move(other.m_Rotation);
		m_Scale = std::move(other.m_Scale);
		m_Translation = std::move(other.m_Translation);
	}

	Transform& Transform::operator=(const Transform& other)
	{
		m_Rotation = other.m_Rotation;
		m_Scale = other.m_Scale;
		m_Translation = other.m_Translation;

		return *this;
	}

	Transform& Transform::operator=(Transform&& other) noexcept
	{
		m_Rotation = std::move(other.m_Rotation);
		m_Scale = std::move(other.m_Scale);
		m_Translation = std::move(other.m_Translation);

		return *this;
	}

	VkTransformMatrixKHR Transform::GetKhrMat()
	{
		glm::mat4 temp = glm::transpose(GetMat4());
		VkTransformMatrixKHR out_matrix;
		memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
		return out_matrix;
	}

	glm::mat4 Transform::GetMat4()
	{
		glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), m_Translation);
		glm::mat4 rotationMat = m_Rotation.GetMat4();
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), m_Scale);

		return translationMat * rotationMat * scaleMat;
	}

	void Transform::SetTranslation(const glm::vec3& vec)
	{
		m_Translation = vec;
	}

	void Transform::SetRotation(const glm::vec3& vec)
	{
		m_Rotation.SetAngles(vec);
	}

	void Transform::SetRotation(const glm::quat& quat)
	{
		m_Rotation.SetQuaternion(quat);
	}

	void Transform::SetScale(const glm::vec3& vec)
	{
		m_Scale = vec;
	}

	void Transform::AddTranslation(const glm::vec3& vec)
	{
		m_Translation += vec;
	}

	void Transform::AddRotation(const glm::vec3& vec)
	{
		m_Rotation.AddAngles(vec);
	}

	void Transform::AddScale(const glm::vec3& vec)
	{
		m_Scale += vec;
	}

}