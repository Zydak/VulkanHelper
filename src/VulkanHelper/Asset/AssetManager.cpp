#include "pch.h"
#include "AssetManager.h"
#include "AssetImporter.h"

namespace VulkanHelper
{
	void AssetManager::Init(const CreateInfo& createInfo)
	{
		if (s_Initialized)
			Destroy();

		s_ThreadPool.Init({ createInfo.ThreadCount });

		s_Initialized = true;
	}

	void AssetManager::Destroy()
	{
		if (!s_Initialized)
			return;

		for (auto iterator = s_Assets.begin();;)
		{
			iterator = s_Assets.begin();

			if (iterator == s_Assets.end())
				break;

			iterator->second.Asset.reset();
			s_Assets.erase(iterator->first);
		}

		s_ThreadPool.Destroy();
		s_Assets.clear();
		s_Initialized = false;
	}

	Asset* AssetManager::GetAsset(const AssetHandle& handle)
	{
		auto iter = s_Assets.find(handle);
		VK_CORE_ASSERT(iter != s_Assets.end(), "There is no such handle!");

		return iter->second.Asset.get();
	}

	bool AssetManager::IsAssetValid(const AssetHandle& handle)
	{
		auto iter = s_Assets.find(handle);
		VK_CORE_ASSERT(iter != s_Assets.end(), "There is no such handle!");

		return iter->second.Asset->IsValid();
	}

	bool AssetManager::DoesHandleExist(const AssetHandle& handle)
	{
		auto iter = s_Assets.find(handle);
		return iter != s_Assets.end();
	}

	void AssetManager::WaitToLoad(const AssetHandle& handle)
	{
		auto iter = s_Assets.find(handle);
		VK_CORE_ASSERT(iter != s_Assets.end(), "There is no such handle!");

		if (iter->second.Future.wait_for(std::chrono::duration<float>(0)) == std::future_status::ready)
			return;

		iter->second.Future.wait();
	}

	bool AssetManager::IsAssetLoaded(const AssetHandle& handle)
	{
		auto iter = s_Assets.find(handle);
		VK_CORE_ASSERT(iter != s_Assets.end(), "There is no such handle!");

		return iter->second.Future.wait_for(std::chrono::duration<float>(0)) == std::future_status::ready;
	}

	AssetHandle AssetManager::LoadAsset(const std::string& path)
	{
		std::hash<std::string> hash;
		AssetHandle handle(AssetHandle::CreateInfo{hash(path)});
		if (s_Assets.contains(handle))
		{
			// Asset with this path is already loaded
			return AssetHandle(handle);
		}

		size_t dotPos = path.find_last_of('.');
		VK_CORE_ASSERT(dotPos != std::string::npos, "Failed to get file extension! Path: {}", path);
		std::string extension = path.substr(dotPos, path.size() - dotPos);

		std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();

		std::unique_lock<std::mutex> lock(s_AssetsMutex);
		s_Assets[handle] = { promise->get_future(), nullptr };
		lock.unlock();

		if (extension == ".png" || extension == ".jpg")
		{
			s_ThreadPool.PushTask([](std::string path, std::shared_ptr<std::promise<void>> promise, AssetHandle handle)
				{
					VK_CORE_TRACE("Loading Texture: {}", path);
					Scope<Asset> asset = std::make_unique<TextureAsset>(std::move(AssetImporter::ImportTexture(path, false)));
					asset->SetValid(true);
					asset->SetPath(path);
		
					std::unique_lock<std::mutex> lock(s_AssetsMutex);
					s_Assets[handle].Asset = std::move(asset);
					lock.unlock();
		
					promise->set_value();
				}, path, promise, handle);
		}
		else if (extension == ".gltf" || extension == ".obj" || extension == ".fbx")
		{
			s_ThreadPool.PushTask([](std::string path, std::shared_ptr<std::promise<void>> promise, AssetHandle handle)
				{
					Scope<Asset> asset = std::make_unique<ModelAsset>(std::move(AssetImporter::ImportModel(path)));
					asset->SetValid(true);
					asset->SetPath(path);
		
					std::unique_lock<std::mutex> lock(s_AssetsMutex);
					s_Assets[handle].Asset = std::move(asset);
					lock.unlock();
		
					promise->set_value();
				}, path, promise, handle);
		}
		else if (extension == ".hdr")
		{
			s_ThreadPool.PushTask([](std::string path, std::shared_ptr<std::promise<void>> promise, AssetHandle handle)
				{
					Scope<Asset> asset = std::make_unique<TextureAsset>(std::move(AssetImporter::ImportTexture(path, true)));
					asset->SetValid(true);
					asset->SetPath(path);
		
					std::unique_lock<std::mutex> lock(s_AssetsMutex);
					s_Assets[handle].Asset = std::move(asset);
					lock.unlock();
		
					promise->set_value();
				}, path, promise, handle);
		}
		else { VK_CORE_ASSERT(false, "Extension not supported! Extension: {}", extension); }

		return AssetHandle(handle);
	}

	VulkanHelper::AssetHandle AssetManager::AddAsset(const std::string& path, std::unique_ptr<Asset>&& asset)
	{
		std::hash<std::string> hash;
		AssetHandle handle(AssetHandle::CreateInfo{ hash(path) });
		if (s_Assets.contains(handle))
		{
			// Asset with this path is already loaded
			asset.reset();
			return AssetHandle(handle);
		}

		std::shared_ptr<std::promise<void>> promise = std::make_shared<std::promise<void>>();

		asset->SetValid(true);
		asset->SetPath(path);

		std::unique_lock<std::mutex> lock(s_AssetsMutex);
		s_Assets[handle] = { promise->get_future(), std::move(asset) };
		lock.unlock();

		promise->set_value();

		return AssetHandle(handle);
	}

	void AssetManager::UnloadAsset(const AssetHandle& handle)
	{
		VK_CORE_TRACE("Unloading asset: {}", handle.GetAsset()->GetPath());

		// Immediately remove asset from m_Assets and then destroy everything on separate thread
		s_AssetsMutex.lock();

		Ref<AssetWithFuture> asset = std::make_shared<AssetWithFuture>(std::move(s_Assets[handle]));
		s_Assets.erase(handle);

		s_AssetsMutex.unlock();
		asset.reset();

		//s_ThreadPool.PushTask([](Ref<AssetWithFuture> asset)
		//	{
		//		asset.reset();
		//	}, asset);
	}

}