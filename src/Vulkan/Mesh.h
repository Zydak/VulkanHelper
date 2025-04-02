#pragma once
#include "Pch.h"

#include "ErrorCodes.h"
#include "vulkan/vulkan.h"
#include "Buffer.h"

namespace VulkanHelper
{
	class Device;

	class Mesh
	{
	public:

		struct InputAttribute
		{
			VkFormat Format = VK_FORMAT_UNDEFINED;
			uint32_t Offset = 0;
		};

		struct CreateInfo
		{
			Device* Device = nullptr;
			std::vector<InputAttribute> InputAttributes;

			void* VertexData = nullptr;
			uint64_t VertexDataSize = 0;

			std::vector<uint32_t> IndexData;

			uint32_t VertexSize = 0;
		};

		ResultCode Init(const CreateInfo& createInfo);
		Mesh() = default;
		~Mesh();

		Mesh(const Mesh& other) = delete;
		Mesh& operator=(const Mesh& other) = delete;
		Mesh(Mesh&& other) noexcept;
		Mesh& operator=(Mesh&& other) noexcept;

		void Bind(VkCommandBuffer commandBuffer) const;
		void Draw(VkCommandBuffer commandBuffer, uint32_t instanceCount, uint32_t firstInstance = 0) const;

	public:

		inline VkBuffer GetVertexBuffer() const { return m_VertexBuffer.GetHandle(); }
		inline uint64_t GetVertexCount() const { return m_VertexCount; }

		inline VkBuffer GetIndexBuffer() const { return m_IndexBuffer.GetHandle(); }
		inline uint64_t GetIndexCount() const { return m_IndexCount; }
		inline bool HasIndexBuffer() const { return m_HasIndexBuffer; }

		inline const std::vector<VkVertexInputAttributeDescription>& GetInputAttributes() const { return InputAttributes; }
		inline const VkVertexInputBindingDescription GetBindingDescription() const { return { 0, m_VertexSize, VK_VERTEX_INPUT_RATE_VERTEX }; }

	private:
		void CreateInputAttributes(const std::vector<InputAttribute>& inputAttributes);
		Device* m_Device = nullptr;

		Buffer m_VertexBuffer;
		uint64_t m_VertexCount = 0;

		bool m_HasIndexBuffer = false;
		Buffer m_IndexBuffer;
		uint64_t m_IndexCount = 0;

		std::vector<VkVertexInputAttributeDescription> InputAttributes;
		uint32_t m_VertexSize = 0;

		void Destroy();
		void Move(Mesh&& other);
	};
}