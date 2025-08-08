#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Enums.h"

#include <future>

#include "Asset.h"
#include "ThreadPool.h"

namespace VulkanHelper
{
    class AssetImporter
    {
    public:
        struct Config
        {
            VulkanHelper::ThreadPool* ThreadPool = nullptr;
        };

        [[nodiscard]] static VulkanHelper::Expected<AssetImporter, VHResult> New(const Config& config);

        AssetImporter(const AssetImporter& other) = delete;
        AssetImporter& operator=(const AssetImporter& other) = delete;

        AssetImporter(AssetImporter&& other) noexcept;
        AssetImporter& operator=(AssetImporter&& other) noexcept;

        ~AssetImporter();

        [[nodiscard]] std::future<Expected<SceneAsset, VHResult>> ImportScene(const std::string& filePath);
        [[nodiscard]] std::future<Expected<TextureAsset, VHResult>> ImportTexture(const std::string& texturePath);

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        AssetImporter(UniquePtr<Impl>&& impl);
    };
}