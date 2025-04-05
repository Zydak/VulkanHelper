#include "Pch.h"

#include "Mesh.h"
#include "Device.h"

#include "assimp/scene.h"
#include "assimp/mesh.h"

VulkanHelper::ResultCode VulkanHelper::Mesh::Init(const CreateInfo& createInfo)
{
	Destroy();

	m_Device = createInfo.Device;

	ResultCode res = ResultCode::Success;

	Buffer::CreateInfo vertexBufferInfo{};
	vertexBufferInfo.Device = m_Device;
	vertexBufferInfo.BufferSize = createInfo.VertexDataSize;
	vertexBufferInfo.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vertexBufferInfo.UsageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	res = m_VertexBuffer.Init(vertexBufferInfo);
	if (res != ResultCode::Success)
		return res;

	res = m_VertexBuffer.WriteToBuffer(createInfo.VertexData, createInfo.VertexDataSize);
	if (res != ResultCode::Success)
		return res;

	m_VertexCount = createInfo.VertexDataSize / createInfo.VertexSize;

	if (createInfo.IndexData.size() > 0)
	{
		Buffer::CreateInfo indexBufferInfo{};
		indexBufferInfo.Device = m_Device;
		indexBufferInfo.BufferSize = (VkDeviceSize)createInfo.IndexData.size() * sizeof(uint32_t);
		indexBufferInfo.MemoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		indexBufferInfo.UsageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		res = m_IndexBuffer.Init(indexBufferInfo);
		if (res != ResultCode::Success)
			return res;

		res = m_IndexBuffer.WriteToBuffer((void*)createInfo.IndexData.data(), (VkDeviceSize)createInfo.IndexData.size() * sizeof(uint32_t));
		if (res != ResultCode::Success)
			return res;

		m_IndexCount = createInfo.IndexData.size();

		m_HasIndexBuffer = true;
	}
	else
		m_HasIndexBuffer = false;

	m_VertexSize = createInfo.VertexSize;
	CreateInputAttributes(createInfo.InputAttributes);

	return res;
}

VulkanHelper::ResultCode VulkanHelper::Mesh::Init(Device* device, aiMesh* mesh, const aiScene* scene, glm::mat4 mat /*= glm::mat4(1.0f)*/)
{
	std::vector<DefaultVertex> vertices;
	std::vector<uint32_t> indices;

	// vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		DefaultVertex vertex;
		glm::vec3 vector;

		// positions
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z;
		vertex.Position = mat * glm::vec4(vector, 1.0f);

		// normals
		if (mesh->HasNormals())
		{
			vector.x = mesh->mNormals[i].x;
			vector.y = mesh->mNormals[i].y;
			vector.z = mesh->mNormals[i].z;
			vertex.Normal = glm::normalize(glm::vec3(mat * glm::vec4(vector, 0.0f)));
		}

		// texture coordinates
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates
		{
			glm::vec2 vec;
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			vec.x = mesh->mTextureCoords[0][i].x;
			vec.y = mesh->mTextureCoords[0][i].y;
			vertex.TexCoord = vec;
		}
		else
			vertex.TexCoord = glm::vec2(0.0f, 0.0f);

		vertices.push_back(vertex);
	}

	// indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	CreateInfo createInfo{};
	createInfo.Device = device;
	createInfo.VertexData = vertices.data();
	createInfo.VertexDataSize = vertices.size() * sizeof(DefaultVertex);
	createInfo.VertexSize = sizeof(DefaultVertex);
	createInfo.InputAttributes = {
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(DefaultVertex, Position) },
		{ VK_FORMAT_R32G32B32_SFLOAT, offsetof(DefaultVertex, Normal) },
		{ VK_FORMAT_R32G32_SFLOAT, offsetof(DefaultVertex, TexCoord) }
	};
	createInfo.IndexData = indices;
	return Init(createInfo);
}

VulkanHelper::Mesh::~Mesh()
{
	Destroy();
}

void VulkanHelper::Mesh::CreateInputAttributes(const std::vector<InputAttribute>& inputAttributes)
{
	InputAttributes.resize(inputAttributes.size());
	for (size_t i = 0; i < inputAttributes.size(); i++)
	{
		InputAttributes[i].location = (uint32_t)i;
		InputAttributes[i].binding = 0;
		InputAttributes[i].format = inputAttributes[i].Format;
		InputAttributes[i].offset = inputAttributes[i].Offset;
	}
}

void VulkanHelper::Mesh::Destroy()
{
	if (m_VertexBuffer.GetHandle() == VK_NULL_HANDLE)
		return;
	
	Reset();
}

void VulkanHelper::Mesh::Move(Mesh&& other)
{
	m_Device = other.m_Device;
	m_VertexBuffer = std::move(other.m_VertexBuffer);
	m_VertexCount = other.m_VertexCount;
	m_HasIndexBuffer = other.m_HasIndexBuffer;
	m_IndexBuffer = std::move(other.m_IndexBuffer);
	m_IndexCount = other.m_IndexCount;
	InputAttributes = std::move(other.InputAttributes);
	m_VertexSize = other.m_VertexSize;

	other.Reset();
}

void VulkanHelper::Mesh::Reset()
{
	m_Device = nullptr;
	m_VertexBuffer = Buffer();
	m_VertexCount = 0;
	m_HasIndexBuffer = false;
	m_IndexBuffer = Buffer();
	m_IndexCount = 0;
	InputAttributes.clear();
	m_VertexSize = 0;
}

VulkanHelper::Mesh& VulkanHelper::Mesh::operator=(Mesh&& other) noexcept
{
	if (this == &other)
		return *this;

	Destroy();

	Move(std::move(other));

	return *this;
}

VulkanHelper::Mesh::Mesh(Mesh&& other) noexcept
{
	if (this == &other)
		return;

	Destroy();

	Move(std::move(other));
}

void VulkanHelper::Mesh::Bind(VkCommandBuffer commandBuffer) const
{
	VkBuffer buffers[] = { m_VertexBuffer.GetHandle() };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (m_HasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetHandle(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void VulkanHelper::Mesh::Draw(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance /*= 0*/) const
{
	if (m_HasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, (uint32_t)m_IndexCount, instanceCount, 0, 0, firstInstance);
	}
	else
	{
		vkCmdDraw(commandBuffer, (uint32_t)m_VertexCount, instanceCount, 0, firstInstance);
	}
}
