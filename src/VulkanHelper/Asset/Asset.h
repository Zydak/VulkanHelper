#pragma once
#include "Utility/Utility.h"
#include <future>
#include "Vulkan/Image.h"
#include "Scene/Scene.h"
#include "Renderer/Mesh.h"

namespace VulkanHelper
{
	class AssetManager;

	enum class AssetType
	{
		Mesh,
		Material,
		Model,
		Texture,
		Scene,
	};

	class Asset
	{
	public:
		virtual ~Asset() {};

		virtual AssetType GetAssetType() = 0;

		inline void SetValid(bool val) { Valid = val; }
		inline bool IsValid() const { return Valid; }
		inline void SetPath(const std::string& path) { Path = path; }
		inline std::string GetPath() { return Path; }
	protected:
		bool Valid = false;
		std::string Path = "";

		friend AssetManager;
	};

	class Material;

	class AssetHandle
	{
	public:
		struct CreateInfo
		{
			uint64_t Handle;
		};

		AssetHandle() = default;
		AssetHandle(const CreateInfo& createInfo);
		~AssetHandle();

		void Init(const CreateInfo& createInfo);
		void Destroy();

		AssetHandle(const AssetHandle& other);
		AssetHandle(AssetHandle&& other) noexcept;
		AssetHandle& operator=(const AssetHandle& other);
		AssetHandle& operator=(AssetHandle&& other) noexcept;

		Asset* GetAsset() const;
		AssetType GetAssetType() const;
		Mesh* GetMesh() const;
		Material* GetMaterial() const;
		Scene* GetScene() const;
		Image* GetImage() const;
		bool IsAssetValid() const;
		bool DoesHandleExist() const;
		bool IsAssetLoaded() const;

		inline bool IsInitialized() const { return m_Initialized; }

		void Unload() const;
		void WaitToLoad() const;

		bool operator==(const AssetHandle& other) const;
		bool operator==(uint64_t other) const;
		size_t Hash() const;
	private:
		uint64_t m_Handle = 0;

		bool m_Initialized = false;

		void Reset();
	};

	class MaterialProperties
	{
	public:
		glm::vec4 Color = glm::vec4(1.0f);
		glm::vec4 EmissiveColor = glm::vec4(1.0f);
		glm::vec4 MediumColor = glm::vec4(1.0f);
		float Metallic = 0.0f;
		float Roughness = 1.0f;
		float SpecularTint = 0.0f;

		float Ior = 1.5f;
		float Transparency = 0.0f;
		float MediumDensity = 1.0f;
		float MediumAnisotropy = 1.0f;

		float Anisotropy = 0.0f;
		float AnisotropyRotation = 0.0f;

		float eta = 0.0f;

		float ax = 1.0f;
		float ay = 1.0f;
	private:
	};

	class MaterialTextures
	{
	public:
		MaterialTextures() {}
		~MaterialTextures();

		explicit MaterialTextures(const MaterialTextures& other) = delete;
		MaterialTextures& operator=(const MaterialTextures& other) = delete;
		explicit MaterialTextures(MaterialTextures&& other) noexcept
		{
			AlbedoTexture = std::move(other.AlbedoTexture);
			NormalTexture = std::move(other.NormalTexture);
			RoughnessTexture = std::move(other.RoughnessTexture);
			MetallnessTexture = std::move(other.MetallnessTexture);
			TexturesSet = std::move(other.TexturesSet);
		}
		MaterialTextures& operator=(MaterialTextures&& other) noexcept
		{
			AlbedoTexture = std::move(other.AlbedoTexture);
			NormalTexture = std::move(other.NormalTexture);
			RoughnessTexture = std::move(other.RoughnessTexture);
			MetallnessTexture = std::move(other.MetallnessTexture);
			TexturesSet = std::move(other.TexturesSet);
			return *this;
		}

		VulkanHelper::DescriptorSet TexturesSet;

		void CreateSet();

		void SetAlbedo(AssetHandle handle);
		void SetNormal(AssetHandle handle);
		void SetRoughness(AssetHandle handle);
		void SetMetallness(AssetHandle handle);

		inline AssetHandle GetAlbedo() const { return AlbedoTexture; }
		inline AssetHandle GetNormal() const { return NormalTexture; }
		inline AssetHandle GetRoughness() const { return RoughnessTexture; }
		inline AssetHandle GetMetallness() const { return MetallnessTexture; }

	private:
		AssetHandle AlbedoTexture;
		AssetHandle NormalTexture;
		AssetHandle RoughnessTexture;
		AssetHandle MetallnessTexture;
	};

	class Material
	{
	public:
		MaterialProperties Properties;
		MaterialTextures Textures;

		std::string MaterialName;
	};

	class TextureAsset : public Asset
	{
	public:
		explicit TextureAsset(Image&& image) { Image = std::move(image); };
		~TextureAsset() {};
		explicit TextureAsset(const TextureAsset& other) = delete;
		TextureAsset& operator=(const TextureAsset& other) = delete;
		explicit TextureAsset(TextureAsset&& other) noexcept { Image = std::move(other.Image); }
		TextureAsset& operator=(TextureAsset&& other) noexcept { Image = std::move(other.Image); return *this; };

		virtual AssetType GetAssetType() override { return AssetType::Texture; }
		VulkanHelper::Image Image;
	};

	class MeshAsset : public Asset
	{
	public:
		explicit MeshAsset(Mesh&& mesh) { Mesh = std::move(mesh); };
		~MeshAsset() {};
		explicit MeshAsset(const MeshAsset& other) = delete;
		MeshAsset& operator=(const MeshAsset& other) = delete;
		explicit MeshAsset(MeshAsset&& other) noexcept { Mesh = std::move(other.Mesh); }
		MeshAsset& operator=(MeshAsset&& other) noexcept { Mesh = std::move(other.Mesh); return *this; };

		virtual AssetType GetAssetType() override { return AssetType::Mesh; }
		VulkanHelper::Mesh Mesh;
	};

	class MaterialAsset : public Asset
	{
	public:
		explicit MaterialAsset(Material&& material) { Material = std::move(material); };
		explicit MaterialAsset(const MaterialAsset& other) = delete;
		MaterialAsset& operator=(const MaterialAsset& other) = delete;
		explicit MaterialAsset(MaterialAsset&& other) noexcept { Material = std::move(other.Material); }
		MaterialAsset& operator=(MaterialAsset&& other) noexcept { Material = std::move(other.Material); return *this; };

		virtual AssetType GetAssetType() override { return AssetType::Material; }
		VulkanHelper::Material Material;
	};

	class ModelAsset : public Asset
	{
	public:
		ModelAsset() = default;
		~ModelAsset() = default;
		explicit ModelAsset(const ModelAsset& other) = delete;
		ModelAsset& operator=(const ModelAsset& other) = delete;
		ModelAsset(ModelAsset&& other) noexcept 
		{ 
			Meshes = std::move(other.Meshes);
			MeshNames = std::move(other.MeshNames);
			Materials = std::move(other.Materials);
			MeshTransfrorms = std::move(other.MeshTransfrorms);
		}
		ModelAsset& operator=(ModelAsset&& other) noexcept 
		{ 
			Meshes = std::move(other.Meshes); 
			Materials = std::move(other.Materials);
			MeshNames = std::move(other.MeshNames);
			MeshTransfrorms = std::move(other.MeshTransfrorms);

			return *this;
		};

		virtual AssetType GetAssetType() override { return AssetType::Model; }
		
		std::vector<AssetHandle> Meshes;
		std::vector<std::string> MeshNames;
		std::vector<glm::mat4> MeshTransfrorms;
		std::vector<AssetHandle> Materials;

		void CreateEntities(VulkanHelper::Scene* outScene, bool addMaterials = true);
	};

	class SceneAsset : public Asset
	{
	public:
		SceneAsset(VulkanHelper::Scene&& scene) { Scene = std::move(scene); };

		explicit SceneAsset(const SceneAsset& other) = delete;
		explicit SceneAsset(SceneAsset&& other) noexcept { Scene = std::move(other.Scene); };
		SceneAsset& operator=(const SceneAsset& other) = delete;
		SceneAsset& operator=(SceneAsset&& other) noexcept { Scene = std::move(other.Scene); return *this; };

		virtual AssetType GetAssetType() override { return AssetType::Scene; }
		VulkanHelper::Scene Scene;
	};
}

namespace std
{
	template<>
	struct std::hash<VulkanHelper::AssetHandle>
	{
		std::size_t operator()(const VulkanHelper::AssetHandle& k) const
		{
			return k.Hash();
		}
	};
}