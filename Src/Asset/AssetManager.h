#pragma once
#include "Pch.h"
#include "Utility/ThreadPool.h"

namespace VulkanHelper
{
	class Device;
	class Asset;
	class AssetManager;

	class AssetHandle
	{
	public:
		inline void WaitToLoad() { Future.wait(); }
		inline [[nodiscard]] std::shared_ptr<Asset> GetAsset() { return Asset; }
	private:
		std::shared_future<void> Future;
		std::shared_ptr<Asset> Asset;
		friend class AssetManager;
	};

	class AssetManager
	{
	public:
		AssetManager() = default;
		~AssetManager() = default;
		static void Init(Device* Device);

		static AssetHandle GetAsset(const std::string& path);

	private:
		inline static Device* s_Device = nullptr;

		struct AssetHandleWeakPtr
		{
			std::shared_future<void> Future;
			std::weak_ptr<Asset> Asset;
		};

		inline static std::unordered_map<uint64_t, AssetHandleWeakPtr> s_Assets;
		inline static ThreadPool s_ThreadPool;
		inline static std::mutex s_AssetsMutex;
	};
}