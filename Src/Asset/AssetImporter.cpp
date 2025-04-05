#pragma once
#include "Pch.h"
#include "AssetImporter.h"
#include "Vulkan/Device.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Logger/Logger.h"

#include "glm.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

VulkanHelper::Image VulkanHelper::AssetImporter::ImportTexture(Device* device, std::string path, bool HDR)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	for (int i = 0; i < path.size(); i++)
	{
		if (path[i] == '%')
			path[i] = ' ';
	}

	int texChannels;
	bool flipOnLoad = !HDR;
	stbi_set_flip_vertically_on_load_thread(flipOnLoad);
	int sizeX, sizeY;
	void* pixels;
	if (HDR)
	{
		pixels = stbi_loadf(path.c_str(), &sizeX, &sizeY, &texChannels, STBI_rgb_alpha);
	}
	else
	{
		pixels = stbi_load(path.c_str(), &sizeX, &sizeY, &texChannels, STBI_rgb_alpha);
	}

	std::filesystem::path cwd = std::filesystem::current_path();
	VH_ASSERT(pixels, "failed to load texture image! Path: {0}, Current working directory: {1}", path, cwd.string());
	uint64_t sizeOfPixel = HDR ? sizeof(float) * 4 : sizeof(uint8_t) * 4;
	VkDeviceSize imageSize = (uint64_t)sizeX * (uint64_t)sizeY * sizeOfPixel;

	Image::CreateInfo info{};
	info.Aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	info.Format = HDR ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM;
	info.Height = sizeY;
	info.Width = sizeX;
	info.Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	info.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	info.MipMapCount = glm::min(5, (int)glm::floor(glm::log2((float)glm::max(sizeX, sizeY))));
	info.Device = device;
	Image image;
	image.Init(info);

	image.WritePixels(pixels, imageSize, true);

	stbi_image_free(pixels);

	return Image(std::move(image));
}

void VulkanHelper::AssetImporter::ImportModel(
	Device* device,
	std::string path,
	std::vector<Mesh>* outMeshes,
	std::vector<std::string>* outMeshNames,
	std::vector<glm::mat4>* outMeshTransfrorms,
	std::vector<Material>* outMaterials
)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path,
		aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_ImproveCacheLocality |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_SplitLargeMeshes |
		aiProcess_Triangulate |
		aiProcess_GenUVCoords |
		aiProcess_SortByPType |
		aiProcess_FindDegenerates |
		aiProcess_FindInvalidData);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		VH_ERROR("Failed to load model: {0}", importer.GetErrorString());
		return;
	}

	int index = 0;
	ProcessAssimpNode(device, scene->mRootNode, scene, path, outMeshes, outMeshNames, outMeshTransfrorms, outMaterials, index);

	for (size_t i = 0; i < outMaterials->size(); i++)
	{
		(*outMaterials)[i].AlbedoTexture.WaitToLoad();
		(*outMaterials)[i].NormalTexture.WaitToLoad();
		(*outMaterials)[i].MetallnessTexture.WaitToLoad();
		(*outMaterials)[i].RoughnessTexture.WaitToLoad();
	}
}

void VulkanHelper::AssetImporter::ProcessAssimpNode(
	Device* device,
	aiNode* node,
	const aiScene* scene,
	const std::string& filepath,
	std::vector<Mesh>* outMeshes,
	std::vector<std::string>* outMeshNames,
	std::vector<glm::mat4>* outMeshTransfrorms,
	std::vector<Material>* outMaterials,
	int& index
)
{
	// process each mesh located at the current node
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		glm::mat4 transform = glm::transpose(*(glm::mat4*)(&node->mTransformation));
		aiNode* currNode = node;
		while (true)
		{
			if (currNode->mParent)
			{
				currNode = currNode->mParent;
				transform = glm::transpose(*(glm::mat4*)(&currNode->mTransformation)) * transform;
			}
			else
			{
				break;
			}
		}

		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		std::string meshName = node->mName.C_Str();

		{
			Mesh vhMesh;
			vhMesh.Init(device, mesh, scene);

			outMeshNames->push_back(meshName);
			outMeshes->emplace_back(std::move(vhMesh));
		}

		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		Material mat;

		aiColor4D emissiveColor(0.0f, 0.0f, 0.0f, 0.0f);
		aiColor4D diffuseColor(0.0f, 0.0f, 0.0f, 0.0f);

		{
			std::string matName = material->GetName().C_Str();

			outMaterials->emplace_back();
			(*outMaterials)[outMaterials->size() - 1].MaterialName = matName;

			material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
			material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveColor.a);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
			material->Get(AI_MATKEY_ROUGHNESS_FACTOR, mat.Roughness);
			material->Get(AI_MATKEY_METALLIC_FACTOR, mat.Metallic);
			material->Get(AI_MATKEY_REFRACTI, mat.Ior);
			material->Get(AI_MATKEY_TRANSMISSION_FACTOR, mat.Transparency);

			mat.Roughness = glm::pow(mat.Roughness, 1.0f / 4.0f);

			for (int i = 0; i < (int)material->GetTextureCount(aiTextureType_DIFFUSE); i++)
			{
				aiString str;
				material->GetTexture(aiTextureType_DIFFUSE, i, &str);
				mat.AlbedoTexture = AssetManager::GetAsset(std::string("assets/") + std::string(str.C_Str()));
			}

			for (int i = 0; i < (int)material->GetTextureCount(aiTextureType_NORMALS); i++)
			{
				aiString str;
				material->GetTexture(aiTextureType_NORMALS, i, &str);
				mat.NormalTexture = AssetManager::GetAsset(std::string("assets/") + std::string(str.C_Str()));
			}

			for (int i = 0; i < (int)material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS); i++)
			{
				aiString str;
				material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, i, &str);
				mat.RoughnessTexture = AssetManager::GetAsset(std::string("assets/") + std::string(str.C_Str()));
			}

			for (int i = 0; i < (int)material->GetTextureCount(aiTextureType_METALNESS); i++)
			{
				aiString str;
				material->GetTexture(aiTextureType_METALNESS, i, &str);
				mat.MetallnessTexture = AssetManager::GetAsset(std::string("assets/") + std::string(str.C_Str()));
			}

			// Create Empty Texture if none are found
			if (material->GetTextureCount(aiTextureType_DIFFUSE) == 0)
			{
				mat.AlbedoTexture = AssetManager::GetAsset("assets/white.png");
			}
			if (material->GetTextureCount(aiTextureType_NORMALS) == 0)
			{
				mat.NormalTexture = AssetManager::GetAsset("assets/empty_normal.png");
			}
			if (material->GetTextureCount(aiTextureType_METALNESS) == 0)
			{
				mat.MetallnessTexture = AssetManager::GetAsset("assets/white.png");
			}
			if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) == 0)
			{
				mat.RoughnessTexture = AssetManager::GetAsset("assets/white.png");
			}

			mat.Color = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f);
			mat.EmissiveColor = glm::vec4(emissiveColor.r, emissiveColor.g, emissiveColor.b, emissiveColor.a);
		}

		index++;
	}

	// process each of the children nodes
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessAssimpNode(device, node->mChildren[i], scene, filepath, outMeshes, outMeshNames, outMeshTransfrorms, outMaterials, index);
	}
}