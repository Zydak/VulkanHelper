#pragma once

#include "Utility/AssetImporter.h"
#include "Core/SharedPtr.h"
#include "Core/Expected.h"
#include "Core/Vector.h"
#include "Utility/ThreadPool.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace VulkanHelper
{
    class AssetImporter::Impl
    {
    public:
        [[nodiscard]] static VulkanHelper::Expected<VulkanHelper::SharedPtr<Impl>, VHResult> New(const Config& config);

        ~Impl();
        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;
        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        [[nodiscard]] inline static Impl* GetImplementation(const AssetImporter& publicInterface) { return publicInterface.m_Impl.Get(); }
        [[nodiscard]] inline static AssetImporter CreatePublicInterface(const VulkanHelper::SharedPtr<Impl>& impl) { return AssetImporter(impl); }

        [[nodiscard]] std::future<Expected<SceneAsset, VHResult>> ImportScene(const std::string& filePath);
        [[nodiscard]] std::future<Expected<TextureAsset, VHResult>> ImportTexture(const std::string& texturePath);

    private:
        explicit Impl(VulkanHelper::ThreadPool* threadPool);
        
        [[nodiscard]] Expected<MeshAsset, VHResult> ProcessMesh(const aiMesh* mesh, const glm::mat4& bakedTransform);
        [[nodiscard]] Expected<MaterialAsset, VHResult> ProcessMaterial(const aiMaterial* material);
        [[nodiscard]] Expected<TextureAsset, VHResult> ProcessTexture(const std::string& texturePath);
        [[nodiscard]] Expected<TextureAsset, VHResult> LoadTexture(const std::string& texturePath);
        
        glm::mat4 ConvertAssimpMatrix(const aiMatrix4x4& assimpMatrix);
        
        [[nodiscard]] Expected<SceneAsset, VHResult> ProcessScene(const aiScene* scene, const std::string& filePath);
        [[nodiscard]] Expected<VulkanHelper::Vector<CameraAsset>, VHResult> ProcessCameras(const aiScene* scene);
        
        [[nodiscard]] VHResult ProcessNode(
            const aiNode* node,
            const aiScene* scene,
            const glm::mat4& parentTransform,
            const std::string& sceneFilePath,
            VulkanHelper::Vector<MeshAsset>& outMeshAssets,
            VulkanHelper::Vector<TextureAsset>& outBaseColorTextures,
            VulkanHelper::Vector<TextureAsset>& outNormalTextures,
            VulkanHelper::Vector<TextureAsset>& outRoughnessTextures,
            VulkanHelper::Vector<TextureAsset>& outMetallicTextures,
            VulkanHelper::Vector<TextureAsset>& outEmissiveTextures,
            VulkanHelper::Vector<MaterialAsset>& outMaterials
        );

        VulkanHelper::ThreadPool* m_ThreadPool;
    };
}