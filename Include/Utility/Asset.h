#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/Enums.h"
#include "Core/Vector.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include "glm/glm.hpp"

namespace VulkanHelper
{
    struct LoadedMeshVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoord;
    };

    struct MeshAsset
    {
        VulkanHelper::Vector<LoadedMeshVertex> Vertices;
        VulkanHelper::Vector<uint32_t> Indices;

        MeshAsset() = default;
        MeshAsset(const MeshAsset&) = delete;
        MeshAsset& operator=(const MeshAsset&) = delete;

        MeshAsset(MeshAsset&& other)
            : Vertices(VulkanHelper::Move(other.Vertices)), Indices(VulkanHelper::Move(other.Indices))
        {}

        MeshAsset& operator=(MeshAsset&& other)
        {
            if (this == &other)
                return *this;

            Vertices = VulkanHelper::Move(other.Vertices);
            Indices = VulkanHelper::Move(other.Indices);

            return *this;
        }

    };

    struct TextureAsset
    {
        VulkanHelper::Vector<uint8_t> Data;
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t Channels = 0;
        bool HighDynamicRange = false;

        TextureAsset() = default;
        TextureAsset(const TextureAsset&) = delete;
        TextureAsset& operator=(const TextureAsset&) = delete;
        TextureAsset(TextureAsset&& other)
            : Data(VulkanHelper::Move(other.Data)), Width(other.Width), Height(other.Height), Channels(other.Channels)
        {
            other.Width = 0;
            other.Height = 0;
            other.Channels = 0;
        }

        TextureAsset& operator=(TextureAsset&& other)
        {
            if (this == &other)
                return *this;

            Data = VulkanHelper::Move(other.Data);
            Width = other.Width;
            Height = other.Height;
            Channels = other.Channels;

            other.Width = 0;
            other.Height = 0;
            other.Channels = 0;

            return *this;
        }
    };

    struct MaterialAsset
    {
        glm::vec3 BaseColor = glm::vec3(1.0f);
        glm::vec3 EmissiveColor = glm::vec3(0.0f);
        float Metallic = 0.0f;
        float Roughness = 1.0f;
        float IOR = 1.5f;
        float Transmission = 0.0f;
        float Anisotropy = 0.0f;
        float AnisotropyRotation = 0.0f;
    };

    struct CameraAsset
    {
        glm::mat4 ViewMatrix = glm::mat4(1.0f);
        float FOV = 45.0f; // Field of View in degrees
        float AspectRatio = 1.0f; // Width / Height
    };

    struct SceneAsset
    {
        VulkanHelper::Vector<MeshAsset> Meshes;
        VulkanHelper::Vector<TextureAsset> BaseColorTextures;
        VulkanHelper::Vector<TextureAsset> NormalTextures;
        VulkanHelper::Vector<TextureAsset> RoughnessTextures;
        VulkanHelper::Vector<TextureAsset> MetallicTextures;
        VulkanHelper::Vector<TextureAsset> EmissiveTextures;
        VulkanHelper::Vector<MaterialAsset> Materials;
        VulkanHelper::Vector<CameraAsset> Cameras;
    };
}