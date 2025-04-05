#pragma once
#include "Pch.h"
#include "AssetManager.h"
#include "Asset.h"

#include "Logger/Logger.h"

#include "Vulkan/Device.h"
#include "AssetImporter.h"

void VulkanHelper::AssetManager::Init(Device* device)
{
	s_Device = device;

	s_ThreadPool.Init({ device, 1 });
}

VulkanHelper::AssetHandle VulkanHelper::AssetManager::GetAsset(const std::string& path)
{
	std::hash<std::string> hash;
	uint64_t hashValue = hash(path);

	size_t dotPos = path.find_last_of('.');
	VH_ASSERT(dotPos != std::string::npos, "Failed to get file extension! Path: {}", path);
	std::string extension = path.substr(dotPos, path.size() - dotPos);

	s_AssetsMutex.lock();
	auto iter = s_Assets.find(hashValue);
	if (iter != s_Assets.end())
	{
		// Asset with this path is already loaded
		if (!iter->second.Asset.expired())
		{
			AssetHandle handle;
			handle.Future = iter->second.Future;
			handle.Asset = iter->second.Asset.lock();
			s_AssetsMutex.unlock();
			return handle;
		}
	}

	std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();
	
	AssetHandle handle;
	handle.Future = promise->get_future();
	if (extension == ".png" || extension == ".jpg")
		handle.Asset = std::make_shared<TextureAsset>();
	else if (extension == ".gltf" || extension == ".glb" || extension == ".obj")
		handle.Asset = std::make_shared<ModelAsset>();
	else
	{
		VH_ASSERT(false, "Unsupported file extension! Path: {}", path);
	}

	s_Assets[hashValue] = { handle.Future, handle.Asset };
	s_AssetsMutex.unlock();

	if (extension == ".png" || extension == ".jpg")
	{
		s_ThreadPool.PushTask([](std::string path, std::shared_ptr<std::promise<void>> promise)
			{
				VH_TRACE("Loading Texture: {}", path);

				std::hash<std::string> hash;
				uint64_t hashValue = hash(path);

				s_AssetsMutex.lock();
				if (s_Assets[hashValue].Asset.expired()) // User deleted the handle during loading
				{
					promise->set_value();
					s_AssetsMutex.unlock();
					return;
				}
				s_AssetsMutex.unlock();

				TextureAsset* textureAsset = (TextureAsset*)s_Assets[hashValue].Asset.lock().get();
				textureAsset->Image = std::move(AssetImporter::ImportTexture(s_Device, path, false));

				promise->set_value();
			}, path, promise);
	}
	else if (extension == ".gltf" || extension == ".glb" || extension == ".obj")
	{
		s_ThreadPool.PushTask([](std::string path, std::shared_ptr<std::promise<void>> promise)
			{
				VH_TRACE("Loading Model: {}", path);
				std::hash<std::string> hash;
				uint64_t hashValue = hash(path);

				s_AssetsMutex.lock();
				if (s_Assets[hashValue].Asset.expired()) // User deleted the handle during loading
				{
					promise->set_value();
					s_AssetsMutex.unlock();
					return;
				}
				s_AssetsMutex.unlock();

				ModelAsset* modelAsset = (ModelAsset*)s_Assets[hashValue].Asset.lock().get();
				AssetImporter::ImportModel(s_Device, path, &modelAsset->Meshes, &modelAsset->MeshNames, &modelAsset->MeshTransfrorms, &modelAsset->Materials);
				promise->set_value();
			}, path, promise);
	}

	return handle;
}
