#pragma once
#include "Pch.h"

#include "Vulkan/Image.h"
#include "Vulkan/Mesh.h"
#include "AssetManager.h"
#include "glm.hpp"

namespace VulkanHelper
{
	class Asset
	{
	public:
		Asset() = default;
		~Asset() = default;
	};

	class TextureAsset : public Asset
	{
	public:
		TextureAsset() = default;
		~TextureAsset() = default;
		Image Image;
	};

	class Material
	{
	public:
		Material() = default;
		~Material() = default;

		glm::vec4 Color = glm::vec4(1.0f);
		glm::vec4 EmissiveColor = glm::vec4(1.0f);
		glm::vec4 MediumColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
		float Metallic = 0.0f;
		float Roughness = 1.0f;
		float SpecularTint = 0.0f;

		float Ior = 1.5f;
		float Transparency = 0.0f;
		float MediumDensity = 1.0f;
		float MediumAnisotropy = 1.0f;

		float Anisotropy = 0.0f;
		float AnisotropyRotation = 0.0f;

		glm::vec3 padding;

		AssetHandle AlbedoTexture;
		AssetHandle NormalTexture;
		AssetHandle RoughnessTexture;
		AssetHandle MetallnessTexture;

		std::string MaterialName;
	};

	class ModelAsset : public Asset
	{
	public:
		ModelAsset() = default;
		~ModelAsset() = default;

		std::vector<Mesh> Meshes;
		std::vector<std::string> MeshNames;
		std::vector<glm::mat4> MeshTransfrorms;
		std::vector<Material> Materials;
	};
}