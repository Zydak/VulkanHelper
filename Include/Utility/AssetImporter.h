#pragma once

#include "Core/SharedPtr.h"
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

        AssetImporter();
        
        AssetImporter(const AssetImporter& other);
        AssetImporter& operator=(const AssetImporter& other);

        AssetImporter(AssetImporter&& other) noexcept;
        AssetImporter& operator=(AssetImporter&& other) noexcept;

        ~AssetImporter();

        [[nodiscard]] std::future<Expected<SceneAsset, VHResult>> ImportScene(const std::string& filePath);
        [[nodiscard]] std::future<Expected<TextureAsset, VHResult>> ImportTexture(const std::string& texturePath);

        class Impl;
    private:
        friend class Impl;
        SharedPtr<Impl> m_Impl;

        AssetImporter(const SharedPtr<Impl>& impl);
    };
}