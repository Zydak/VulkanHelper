#include "AssetImporterImpl.h"
#include "Log/Log.h"
#include "Core/Move.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace VulkanHelper
{
    Expected<SharedPtr<AssetImporter::Impl>, VHResult> AssetImporter::Impl::New(const Config& config)
    {
        VH_LOG_INFO("Creating AssetImporter Implementation");

        if (!config.ThreadPool)
        {
            VH_LOG_ERROR("ThreadPool cannot be null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        return SharedPtr<Impl>(new Impl(config.ThreadPool));
    }

    AssetImporter::Impl::Impl(VulkanHelper::ThreadPool* threadPool)
        : m_ThreadPool(threadPool)
    {

    }

    AssetImporter::Impl::~Impl()
    {
        if (m_ThreadPool != nullptr)
        {
            VH_LOG_DEBUG("Destroying AssetImporter Implementation");
            m_ThreadPool = nullptr;
        }
    }

    AssetImporter::Impl::Impl(Impl&& other) noexcept
        : m_ThreadPool(other.m_ThreadPool)
    {
        other.m_ThreadPool = nullptr;
    }

    AssetImporter::Impl& AssetImporter::Impl::operator=(Impl&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~Impl(); // Clean up current state

        m_ThreadPool = other.m_ThreadPool;
        other.m_ThreadPool = nullptr;

        return *this;
    }

    std::future<Expected<SceneAsset, VHResult>> AssetImporter::Impl::ImportScene(const std::string& filePath)
    {
        // Submit the complete scene import task to the thread pool
        return m_ThreadPool->Submit([this, filePath]() -> Expected<SceneAsset, VHResult> {
            VH_LOG_DEBUG("Starting scene import for file: {}", filePath);

            Assimp::Importer importer;

            // Configure post-processing steps for complete scene loading
            unsigned int postProcessFlags = 
                aiProcess_Triangulate |              // Convert all faces to triangles
                aiProcess_GenNormals |               // Generate normals if missing
                aiProcess_GenUVCoords |              // Generate UV coordinates if missing
                aiProcess_CalcTangentSpace |         // Calculate tangent space
                aiProcess_JoinIdenticalVertices |    // Join identical vertices
                aiProcess_SortByPType |              // Sort primitives by type
                aiProcess_OptimizeMeshes |           // Optimize mesh count
                aiProcess_OptimizeGraph |            // Optimize scene graph
                aiProcess_FlipUVs;                   // Flip UV coordinates for OpenGL/Vulkan

            // Load the file
            const aiScene* scene = importer.ReadFile(filePath, postProcessFlags);

            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                VH_LOG_ERROR("Failed to load scene file: {} - Error: {}", filePath, importer.GetErrorString());
                return Unexpected(VHResult::INITIALIZATION_FAILED);
            }

            VH_LOG_DEBUG("Successfully loaded scene file: {} with {} meshes, {} materials, {} cameras", 
                        filePath, scene->mNumMeshes, scene->mNumMaterials, scene->mNumCameras);

            return ProcessScene(scene, filePath);
        });
    }

    std::future<Expected<TextureAsset, VHResult>> AssetImporter::Impl::ImportTexture(const std::string& texturePath)
    {
        // Submit the texture import task to the thread pool
        return m_ThreadPool->Submit([this, texturePath]() -> Expected<TextureAsset, VHResult> {
            VH_LOG_DEBUG("Starting texture import for file: {}", texturePath);
            return LoadTexture(texturePath);
        });
    }

    Expected<MeshAsset, VHResult> AssetImporter::Impl::ProcessMesh(const aiMesh* mesh, const glm::mat4& bakedTransform)
    {
        if (!mesh)
        {
            VH_LOG_ERROR("Mesh pointer is null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VH_LOG_DEBUG("Processing mesh with {} vertices and {} faces", mesh->mNumVertices, mesh->mNumFaces);

        MeshAsset meshAsset;
        meshAsset.Vertices.Reserve(mesh->mNumVertices);

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
        {
            LoadedMeshVertex vertex{};

            // Position (always present)
            if (mesh->HasPositions())
            {
                glm::vec3 position = glm::vec3(
                    mesh->mVertices[i].x,
                    mesh->mVertices[i].y,
                    mesh->mVertices[i].z
                );
                
                // Apply transform to position
                glm::vec4 transformedPos = bakedTransform * glm::vec4(position, 1.0f);
                vertex.Position = glm::vec3(transformedPos);
            }

            // Normals (may be generated by post-processing)
            if (mesh->HasNormals())
            {
                glm::vec3 normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
                
                // Apply transform to normal (use inverse transpose for correct normal transformation)
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(bakedTransform)));
                vertex.Normal = glm::normalize(normalMatrix * normal);
            }
            else
            {
                // Apply transform to default normal as well
                glm::vec3 defaultNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(bakedTransform)));
                vertex.Normal = glm::normalize(normalMatrix * defaultNormal);
            }

            // Texture coordinates (use first texture coordinate set if available)
            if (mesh->HasTextureCoords(0))
            {
                vertex.TexCoord = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            }
            else
            {
                vertex.TexCoord = glm::vec2(0.0f, 0.0f); // Default UV
            }

            meshAsset.Vertices.PushBack(vertex);
        }

        // Process indices
        meshAsset.Indices.Reserve(mesh->mNumFaces * 3); // Assuming triangulated faces

        for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
        {
            aiFace face = mesh->mFaces[i];
            
            if (face.mNumIndices != 3)
            {
                VH_LOG_WARN("Face {} has {} indices instead of 3. Skipping non-triangular face.", i, face.mNumIndices);
                continue;
            }

            for (unsigned int j = 0; j < face.mNumIndices; ++j)
            {
                meshAsset.Indices.PushBack(face.mIndices[j]);
            }
        }

        return meshAsset;
    }

    VHResult AssetImporter::Impl::ProcessNode(
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
    )
    {
        // Convert Assimp matrix to glm matrix and combine with parent transform
        glm::mat4 nodeTransform = ConvertAssimpMatrix(node->mTransformation);
        glm::mat4 finalTransform = parentTransform * nodeTransform;

        // Process all meshes in this node
        for (unsigned int i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

            auto meshResult = ProcessMesh(mesh, finalTransform);
            if (!meshResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process mesh '{}'", mesh->mName.C_Str());
                return meshResult.Error();
            }
            auto materialResult = ProcessMaterial(scene->mMaterials[mesh->mMaterialIndex]);
            if (!materialResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process material for mesh '{}'", mesh->mName.C_Str());
                return materialResult.Error();
            }

            aiString texturePath;

            // Base color
            scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
            auto baseColorTextureResult = ProcessTexture(texturePath.Empty() ? "" : sceneFilePath + '/' + texturePath.C_Str());
            if (!baseColorTextureResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process albedo texture for mesh '{}'", mesh->mName.C_Str());
                return baseColorTextureResult.Error();
            }

            // Normal
            scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_NORMALS, 0, &texturePath);
            auto normalTextureResult = ProcessTexture(texturePath.Empty() ? "" : sceneFilePath + '/' + texturePath.C_Str());
            if (!normalTextureResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process normal texture for mesh '{}'", mesh->mName.C_Str());
                return normalTextureResult.Error();
            }

            // Roughness
            scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath);
            auto roughnessTextureResult = ProcessTexture(texturePath.Empty() ? "" : sceneFilePath + '/' + texturePath.C_Str());
            if (!roughnessTextureResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process roughness texture for mesh '{}'", mesh->mName.C_Str());
                return roughnessTextureResult.Error();
            }

            // Metallic
            scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_METALNESS, 0, &texturePath);
            auto metallicTextureResult = ProcessTexture(texturePath.Empty() ? "" : sceneFilePath + '/' + texturePath.C_Str());
            if (!metallicTextureResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process metallic texture for mesh '{}'", mesh->mName.C_Str());
                return metallicTextureResult.Error();
            }

            // Emissive
            scene->mMaterials[mesh->mMaterialIndex]->GetTexture(aiTextureType_EMISSION_COLOR, 0, &texturePath);
            auto emissiveTextureResult = ProcessTexture(texturePath.Empty() ? "" : sceneFilePath + '/' + texturePath.C_Str());
            if (!emissiveTextureResult.HasValue())
            {
                VH_LOG_ERROR("Failed to process emissive texture for mesh '{}'", mesh->mName.C_Str());
                return emissiveTextureResult.Error();
            }

            outMeshAssets.PushBack(Move(meshResult.Value()));
            outBaseColorTextures.PushBack(Move(baseColorTextureResult.Value()));
            outNormalTextures.PushBack(Move(normalTextureResult.Value()));
            outRoughnessTextures.PushBack(Move(roughnessTextureResult.Value()));
            outMetallicTextures.PushBack(Move(metallicTextureResult.Value()));
            outEmissiveTextures.PushBack(Move(emissiveTextureResult.Value()));
            outMaterials.PushBack(Move(materialResult.Value()));
        }

        // Recursively process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; ++i)
        {
            VHResult result = ProcessNode(node->mChildren[i], scene, finalTransform, sceneFilePath, outMeshAssets, outBaseColorTextures, outNormalTextures, outRoughnessTextures, outMetallicTextures, outEmissiveTextures, outMaterials);
            if (result != VHResult::OK)
            {
                VH_LOG_ERROR("Failed to process child node '{}'", node->mChildren[i]->mName.C_Str());
                return result;
            }
        }

        return VHResult::OK;
    }

    glm::mat4 AssetImporter::Impl::ConvertAssimpMatrix(const aiMatrix4x4& assimpMatrix)
    {
        // Assimp matrices are row-major, GLM matrices are column-major
        // We need to transpose when converting
        
        // Rotate on y axis
        glm::mat4 vulkanTransform = glm::mat4(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, -1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );

        return vulkanTransform * glm::mat4(
            assimpMatrix.a1, assimpMatrix.b1, assimpMatrix.c1, assimpMatrix.d1,
            assimpMatrix.a2, assimpMatrix.b2, assimpMatrix.c2, assimpMatrix.d2,
            assimpMatrix.a3, assimpMatrix.b3, assimpMatrix.c3, assimpMatrix.d3,
            assimpMatrix.a4, assimpMatrix.b4, assimpMatrix.c4, assimpMatrix.d4
        );
    }

    Expected<SceneAsset, VHResult> AssetImporter::Impl::ProcessScene(const aiScene* scene, const std::string& filePath)
    {
        if (!scene)
        {
            VH_LOG_ERROR("Scene is null");
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        SceneAsset sceneAsset;

        // Extract base path for texture loading
        std::string basePath = filePath.substr(0, filePath.find_last_of("/\\"));
        if (basePath == filePath) basePath = "."; // No directory separator found

        VHResult res = ProcessNode(
            scene->mRootNode,
            scene,
            glm::mat4(1.0f), // Start with identity matrix
            basePath,
            sceneAsset.Meshes,
            sceneAsset.BaseColorTextures,
            sceneAsset.NormalTextures,
            sceneAsset.RoughnessTextures,
            sceneAsset.MetallicTextures,
            sceneAsset.EmissiveTextures,
            sceneAsset.Materials
        );
        if (res != VHResult::OK)
        {
            VH_LOG_ERROR("Failed to process scene node");
            return Unexpected(res);
        }

        // Process cameras
        auto cameraResult = ProcessCameras(scene);
        if (!cameraResult.HasValue())
        {
            VH_LOG_ERROR("Failed to process cameras");
            return Unexpected(cameraResult.Error());
        }

        sceneAsset.Cameras = Move(cameraResult.Value());

        VH_LOG_DEBUG("Successfully processed complete scene: {} with {} meshes, {} textures, {} materials, {} cameras",
                    filePath, sceneAsset.Meshes.Size(), sceneAsset.BaseColorTextures.Size(), sceneAsset.Materials.Size(), sceneAsset.Cameras.Size());

        return sceneAsset;
    }

    Expected<MaterialAsset, VHResult> AssetImporter::Impl::ProcessMaterial(const aiMaterial* material)
    {
        if (!material)
        {
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        MaterialAsset materialAsset;

        // Get base color (diffuse color)
        aiColor3D baseColor;
        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS)
            materialAsset.BaseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
        else
            materialAsset.BaseColor = glm::vec3(1.0f, 1.0f, 1.0f); // Default value

        aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS)
            materialAsset.EmissiveColor = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
        else
            materialAsset.EmissiveColor = glm::vec3(0.0f, 0.0f, 0.0f); // Default value

        float emissionStrength;
        if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissionStrength) != AI_SUCCESS)
            emissionStrength = 1.0f; // Default value
        materialAsset.EmissiveColor *= emissionStrength;

        float metallic;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
            materialAsset.Metallic = metallic;
        else
            materialAsset.Metallic = 0.0f; // Default value

        float roughness;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
            materialAsset.Roughness = roughness;
        else
            materialAsset.Roughness = 1.0f; // Default value

        float ior;
        if (material->Get(AI_MATKEY_REFRACTI, ior) == AI_SUCCESS)
            materialAsset.IOR = ior;
        else
            materialAsset.IOR = 1.0f; // Default value

        float transmission;
        if (material->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission) == AI_SUCCESS)
            materialAsset.Transmission = transmission;
        else
            materialAsset.Transmission = 0.0f; // Default value

        float anisotropy;
        if (material->Get(AI_MATKEY_ANISOTROPY_FACTOR, anisotropy) == AI_SUCCESS)
            materialAsset.Anisotropy = anisotropy;
        else
            materialAsset.Anisotropy = 0.0f; // Default value

        float anisotropyRotation;
        if (material->Get(AI_MATKEY_ANISOTROPY_ROTATION, anisotropyRotation) == AI_SUCCESS)
            materialAsset.AnisotropyRotation = anisotropyRotation;
        else
            materialAsset.AnisotropyRotation = 0.0f; // Default value

        return materialAsset;
    }

    Expected<TextureAsset, VHResult> AssetImporter::Impl::ProcessTexture(const std::string& texturePath)
    {
        if (texturePath.empty())
        {
            TextureAsset emptyTexture{};
            emptyTexture.Width = 1;
            emptyTexture.Height = 1;
            emptyTexture.Channels = 4; // Default to a 1x1 white texture
            emptyTexture.Data.Resize(4, 255); // Fill with white color (RGBA)
            return emptyTexture;
        }

        // Load the texture using stb_image
        auto textureResult = LoadTexture(texturePath);
        if (!textureResult.HasValue())
        {
            VH_LOG_ERROR("Failed to load texture: {}", texturePath);
            return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        VH_LOG_DEBUG("Successfully loaded texture: {}", texturePath);
        return Move(textureResult.Value());
    }

    Expected<TextureAsset, VHResult> AssetImporter::Impl::LoadTexture(const std::string& texturePath)
    {
        int width, height, channels;
        void* data;
        bool isHDR = (texturePath.find(".hdr") != std::string::npos);
        if (isHDR)
        {
            data = stbi_loadf(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        }
        else
        {
            data = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
        }
         
        // Force 4 channels (RGBA) that's because vulkan really rarely supports 3 channel images, it's really bad for tiling
        channels = 4;
        
        if (!data)
        {
            VH_LOG_ERROR("Failed to load texture: {} - Error: {}", texturePath, stbi_failure_reason());
            return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        // Validate dimensions are positive
        if (width <= 0 || height <= 0 || channels <= 0)
        {
            VH_LOG_ERROR("Invalid texture dimensions: {}x{} with {} channels", width, height, channels);
            stbi_image_free(data);
            return Unexpected(VHResult::INITIALIZATION_FAILED);
        }

        // Calculate the total size with proper casting
        size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
        if (isHDR)
            dataSize *= sizeof(float); // HDR textures use float per channel
        
        TextureAsset textureAsset{};
        textureAsset.Width = static_cast<uint32_t>(width);
        textureAsset.Height = static_cast<uint32_t>(height);
        textureAsset.Channels = static_cast<uint32_t>(channels);
        textureAsset.HighDynamicRange = isHDR;
        textureAsset.Data.Resize(dataSize);
        
        // Copy data to our vector
        memcpy(textureAsset.Data.Data(), data, dataSize);

        // Free stbi memory
        stbi_image_free(data);

        VH_LOG_DEBUG("Loaded texture: {}x{} with {} channels ({} bytes)", width, height, channels, dataSize);
        return textureAsset;
    }

    Expected<VulkanHelper::Vector<CameraAsset>, VHResult> AssetImporter::Impl::ProcessCameras(const aiScene* scene)
    {
        if (!scene)
        {
            return Unexpected(VHResult::WRONG_ARGUMENTS);
        }

        VulkanHelper::Vector<CameraAsset> cameras;
        cameras.Reserve(scene->mNumCameras);

        VH_LOG_DEBUG("Processing {} cameras", scene->mNumCameras);

        for (unsigned int i = 0; i < scene->mNumCameras; ++i)
        {
            aiCamera* camera = scene->mCameras[i];
            
            if (!camera)
            {
                VH_LOG_WARN("Encountered null camera at index {}", i);
                continue;
            }

            // Find the camera node in the scene graph to get its transformation
            glm::mat4 cameraTransform = glm::mat4(1.0f);
            
            // Recursive search for camera node
            auto findCameraNode = [&](const aiNode* node, const std::string& cameraName, glm::mat4 currentTransform) -> bool {
                std::function<bool(const aiNode*, const std::string&, glm::mat4)> search = 
                    [&](const aiNode* currentNode, const std::string& name, glm::mat4 transform) -> bool {
                    
                    if (!currentNode) return false;
                    
                    glm::mat4 nodeTransform = ConvertAssimpMatrix(currentNode->mTransformation);
                    glm::mat4 newTransform = transform * nodeTransform;
                    
                    if (std::string(currentNode->mName.C_Str()) == name)
                    {
                        cameraTransform = newTransform;
                        return true;
                    }
                    
                    for (unsigned int j = 0; j < currentNode->mNumChildren; ++j)
                    {
                        if (search(currentNode->mChildren[j], name, newTransform))
                            return true;
                    }
                    
                    return false;
                };
                
                return search(node, cameraName, currentTransform);
            };

            std::string cameraName = camera->mName.C_Str();
            findCameraNode(scene->mRootNode, cameraName, glm::mat4(1.0f));

            glm::vec3 localPosition = glm::vec3(camera->mPosition.x, camera->mPosition.y, camera->mPosition.z);
            glm::vec3 localUp = glm::normalize(glm::vec3(-camera->mUp.x, -camera->mUp.y, -camera->mUp.z));
            glm::vec3 localLookAt = glm::normalize(glm::vec3(camera->mLookAt.x, camera->mLookAt.y, camera->mLookAt.z));

            glm::vec3 right = glm::normalize(glm::cross(localLookAt, localUp));
            glm::vec3 up = glm::cross(right, localLookAt);
            
            glm::mat4 localCameraMatrix = glm::mat4(1.0f);
            localCameraMatrix[0] = glm::vec4(-right, 0.0f);
            localCameraMatrix[1] = glm::vec4(up, 0.0f);
            localCameraMatrix[2] = glm::vec4(-localLookAt, 0.0f);
            localCameraMatrix[3] = glm::vec4(localPosition, 1.0f);

            glm::mat4 vulkanTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            );

            cameraTransform = vulkanTransform * cameraTransform;

            // Apply world transformation
            glm::mat4 finalCameraMatrix = (cameraTransform * localCameraMatrix);
            
            CameraAsset cameraAsset;
            cameraAsset.ViewMatrix = glm::inverse(finalCameraMatrix);
            cameraAsset.AspectRatio = camera->mAspect > 0.0f ? camera->mAspect : 1.0f; // Default to 1.0 if aspect ratio is not defined

            // glm expects vertical FOV but assimp gives us horizontal
            float vFOV = 2.0f * atan(tan(camera->mHorizontalFOV / 2.0f) / cameraAsset.AspectRatio);
            cameraAsset.FOV = vFOV * (180.0f / 3.14159265358979323846f); // Convert radians to degrees
            cameras.PushBack(cameraAsset);
        }

        VH_LOG_INFO("Successfully processed {} cameras", cameras.Size());
        return cameras;
    }

    //
    // Forward Functions
    //

    Expected<AssetImporter, VHResult> AssetImporter::New(const Config& config)
    {
        VH_LOG_INFO("Creating AssetImporter");

        auto implResult = Impl::New(config);
        if (!implResult.HasValue())
        {
            return Unexpected(implResult.Error());
        }

        return AssetImporter(Move(implResult.Value()));
    }

    AssetImporter::AssetImporter(const SharedPtr<Impl>& impl)
        : m_Impl(impl)
    {
    }

    AssetImporter::~AssetImporter()
    {

    }

    AssetImporter::AssetImporter(const AssetImporter& other)
        : m_Impl(other.m_Impl)
    {
    }

    AssetImporter::AssetImporter(AssetImporter&& other) noexcept
        : m_Impl(Move(other.m_Impl))
    {
    }

    AssetImporter& AssetImporter::operator=(const AssetImporter& other)
    {
        if (this == &other)
            return *this;

        this->~AssetImporter(); // Clean up current state

        m_Impl = other.m_Impl;

        return *this;
    }

    AssetImporter& AssetImporter::operator=(AssetImporter&& other) noexcept
    {
        if (this == &other)
            return *this;

        this->~AssetImporter(); // Clean up current state
        
        m_Impl = Move(other.m_Impl);

        return *this;
    }

    std::future<Expected<SceneAsset, VHResult>> AssetImporter::ImportScene(const std::string& filePath)
    {
        return m_Impl->ImportScene(filePath);
    }

    std::future<Expected<TextureAsset, VHResult>> AssetImporter::ImportTexture(const std::string& texturePath)
    {
        return m_Impl->ImportTexture(texturePath);
    }
}