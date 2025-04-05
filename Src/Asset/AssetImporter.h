#pragma once
#include "Pch.h"

#include "Vulkan/Image.h"
#include "Asset.h"

struct aiNode;
struct aiScene;

namespace VulkanHelper
{
	class AssetManager;
	class Device;

	class AssetImporter
	{
	public:
		static Image ImportTexture(Device* device, std::string path, bool HDR);
		static void ImportModel(
			Device* device,
			std::string path,
			std::vector<Mesh>* outMeshes,
			std::vector<std::string>* outMeshNames,
			std::vector<glm::mat4>* outMeshTransfrorms,
			std::vector<Material>* outMaterials
		);

	private:
		static void ProcessAssimpNode(
			Device* device,
			aiNode* node,
			const aiScene* scene,
			const std::string& filepath,
			std::vector<Mesh>* outMeshes,
			std::vector<std::string>* outMeshNames,
			std::vector<glm::mat4>* outMeshTransfrorms,
			std::vector<Material>* outMaterials,
			int& index
		);
	};

}