#include "pch.h"
#include "Asset.h"
#include <unordered_map>
#include "AssetManager.h"

#include "glm/gtx/matrix_decompose.hpp"

#include "Renderer/Renderer.h"

#include "Core/Window.h"

namespace VulkanHelper
{

	AssetHandle::AssetHandle(const CreateInfo& createInfo)
	{
		Init(createInfo);
	}

	AssetHandle::AssetHandle(const AssetHandle& other)
	{
		Init({ other.m_Handle });
	}

	AssetHandle::AssetHandle(AssetHandle&& other) noexcept
	{
		if (m_Initialized)
			Destroy();

		m_Initialized	= std::move(other.m_Initialized);
		m_Handle		= std::move(other.m_Handle);

		other.Reset();
	}

	AssetHandle::~AssetHandle()
	{
		Destroy();
	}

	void AssetHandle::Init(const CreateInfo& createInfo)
	{
		if (m_Initialized)
			Destroy();

		m_Handle = createInfo.Handle;

		m_Initialized = true;

		// Increase reference count
		if (!AssetManager::s_AssetsReferenceCount.contains(m_Handle))
			AssetManager::s_AssetsReferenceCount[m_Handle] = 1;
		else
			AssetManager::s_AssetsReferenceCount[m_Handle] += 1;
	}

	void AssetHandle::Destroy()
	{
		if (!m_Initialized)
			return;

		// Decrease reference count
		AssetManager::s_AssetsReferenceCount[m_Handle] -= 1;

		// If there are no references then unload the asset
		// <= 2 because 1 handle is stored inside the m_Assets map and one in Asset itself
		if (AssetManager::s_AssetsReferenceCount[m_Handle] <= 2 && DoesHandleExist())
			AssetManager::UnloadAsset(*this);

		m_Initialized = false;
	}

	AssetHandle& AssetHandle::operator=(const AssetHandle& other)
	{
		Init({ other.m_Handle });
		return *this;
	}

	AssetHandle& AssetHandle::operator=(AssetHandle&& other) noexcept
	{
		if (m_Initialized)
			Destroy();

		m_Initialized	= std::move(other.m_Initialized);
		m_Handle		= std::move(other.m_Handle);

		other.Reset();

		return *this;
	}

	Asset* AssetHandle::GetAsset() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return AssetManager::GetAsset(*this);
	}

	AssetType AssetHandle::GetAssetType() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return  AssetManager::GetAsset(*this)->GetAssetType();
	}

	Mesh* AssetHandle::GetMesh() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return &dynamic_cast<MeshAsset*>(AssetManager::GetAsset(*this))->Mesh;
	}

	Material* AssetHandle::GetMaterial() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return &dynamic_cast<MaterialAsset*>(AssetManager::GetAsset(*this))->Material;
	}

	Scene* AssetHandle::GetScene() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return &dynamic_cast<SceneAsset*>(AssetManager::GetAsset(*this))->Scene;
	}

	Image* AssetHandle::GetImage() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return &dynamic_cast<TextureAsset*>( AssetManager::GetAsset(*this))->Image;
	}

	bool AssetHandle::DoesHandleExist() const
	{
		if (!m_Initialized)
			return false;

		return AssetManager::DoesHandleExist(*this);
	}

	bool AssetHandle::IsAssetLoaded() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return AssetManager::IsAssetLoaded(*this);
	}

	void AssetHandle::Unload() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		 AssetManager::UnloadAsset(*this);
	}

	void AssetHandle::WaitToLoad() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		 AssetManager::WaitToLoad(*this);
	}

	size_t AssetHandle::Hash() const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return (size_t)m_Handle;
	}

	void AssetHandle::Reset()
	{
		m_Handle = 0;
		m_Initialized = false;
	}

	bool AssetHandle::operator==(const AssetHandle& other) const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return m_Handle == other.m_Handle;
	}

	bool AssetHandle::operator==(uint64_t other) const
	{
		VK_CORE_ASSERT(m_Initialized, "Handle is not Initialized!");

		return m_Handle == other;
	}

	ModelAsset::ModelAsset(const std::string& path)
		: Asset(AssetManager::CreateHandleFromPath(path), path)
	{

	}

	void ModelAsset::CreateEntities(VulkanHelper::Scene* outScene, VulkanHelperContext context, VkSampler texturesSamplerHandle, bool addMaterials)
	{
		for (int i = 0; i < Meshes.size(); i++)
		{
			VulkanHelper::Entity entity = outScene->CreateEntity();

			VulkanHelper::MeshComponent* meshComp = &entity.AddComponent<MeshComponent>();
			VulkanHelper::Mesh* mesh = Meshes[i].GetMesh();
			meshComp->AssetHandle = Meshes[i];

			auto& nameComp = entity.AddComponent<NameComponent>();
			nameComp.Name = MeshNames[i];

			if (addMaterials)
			{
				auto& materialComp = entity.AddComponent<MaterialComponent>();
				materialComp.AssetHandle = Materials[i];
			}

			glm::mat4 modelMatrix{ 1.0f };
			glm::vec3 translation{};
			glm::vec3 scale{};
			glm::vec3 skew; // don't care
			glm::vec4 perspective; // don't care

			glm::quat rotation{};

			glm::decompose(MeshTransfrorms[i], scale, rotation, translation, skew, perspective);

			VulkanHelper::Transform transform;
			transform.SetRotation(rotation);
			transform.SetTranslation(translation);
			transform.SetScale(scale);

			auto& transformComp = entity.AddComponent<TransformComponent>();
			transformComp.Transform = std::move(transform);

			// This automatically waits for textures
			Materials[i].GetMaterial()->Textures.CreateSet(context, texturesSamplerHandle);

			meshComp->AssetHandle.WaitToLoad();
		}
	}

	MaterialTextures::~MaterialTextures()
	{

	};

	void MaterialTextures::CreateSet(VulkanHelperContext context, VkSampler samplerHandle)
	{
		if (TexturesSet.IsInitialized())
			return;

		VulkanHelper::DescriptorSetLayout::Binding bin1{ 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT };
		VulkanHelper::DescriptorSetLayout::Binding bin2{ 1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT };
		VulkanHelper::DescriptorSetLayout::Binding bin3{ 2, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT };
		VulkanHelper::DescriptorSetLayout::Binding bin4{ 3, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT };

		TexturesSet.Init(&context.Window->GetRenderer()->GetDescriptorPool(), { bin1, bin2, bin3, bin4 });

		AlbedoTexture.WaitToLoad();
		TexturesSet.AddImageSampler(
			0,
			{ samplerHandle,
			AlbedoTexture.GetImage()->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		);
		NormalTexture.WaitToLoad();
		TexturesSet.AddImageSampler(
			1,
			{ samplerHandle,
			NormalTexture.GetImage()->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		);
		RoughnessTexture.WaitToLoad();
		TexturesSet.AddImageSampler(
			2,
			{ samplerHandle,
			RoughnessTexture.GetImage()->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		);
		MetallnessTexture.WaitToLoad();
		TexturesSet.AddImageSampler(
			3,
			{ samplerHandle,
			MetallnessTexture.GetImage()->GetImageView(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
		);

		TexturesSet.Build();
	}

	void MaterialTextures::SetAlbedo(AssetHandle handle)
	{
		AlbedoTexture = handle;
	}

	void MaterialTextures::SetNormal(AssetHandle handle)
	{
		NormalTexture = handle;
	}

	void MaterialTextures::SetRoughness(AssetHandle handle)
	{
		RoughnessTexture = handle;
	}

	void MaterialTextures::SetMetallness(AssetHandle handle)
	{
		MetallnessTexture = handle;
	}

	Asset::Asset(const AssetHandle& handle, const std::string& path)
	{
		m_AssetHandle = handle;
		m_Path = path;
	}

	Asset::~Asset()
	{

	}

	MeshAsset::MeshAsset(const std::string& path)
		: Asset(AssetManager::CreateHandleFromPath(path), path)
	{

	}

	MeshAsset::MeshAsset(const std::string& path, VulkanHelper::Mesh&& mesh)
		: Asset(AssetManager::CreateHandleFromPath(path), path), Mesh(std::move(mesh))
	{

	};

	MeshAsset::MeshAsset(MeshAsset&& other) noexcept
		: Asset(other.GetHandle(), other.GetPath()), Mesh(std::move(other.Mesh))
	{

	}

	TextureAsset::TextureAsset(const std::string& path, VulkanHelper::Image&& image)
		: Asset(AssetManager::CreateHandleFromPath(path), path), Image(std::move(image))
	{

	};

	TextureAsset::TextureAsset(const std::string& path)
		: Asset(AssetManager::CreateHandleFromPath(path), path)
	{

	}

	TextureAsset::TextureAsset(TextureAsset&& other) noexcept
		: Asset(other.GetHandle(), other.GetPath()), Image(std::move(other.Image))
	{

	}

	MaterialAsset::MaterialAsset(const std::string& path)
		: Asset(AssetManager::CreateHandleFromPath(path), path)
	{

	}

	MaterialAsset::MaterialAsset(const std::string& path, VulkanHelper::Material&& material)
		: Asset(AssetManager::CreateHandleFromPath(path), path), Material(std::move(material))
	{

	};

	MaterialAsset::MaterialAsset(MaterialAsset&& other) noexcept
		: Asset(other.GetHandle(), other.GetPath()), Material(std::move(other.Material))
	{

	}

	SceneAsset::SceneAsset(const std::string& path, VulkanHelper::Scene&& scene)
		: Asset(AssetManager::CreateHandleFromPath(path), path), Scene(std::move(scene))
	{

	};

	SceneAsset::SceneAsset(SceneAsset&& other) noexcept
		: Asset(other.GetHandle(), other.GetPath()), Scene(std::move(other.Scene))
	{

	}

}