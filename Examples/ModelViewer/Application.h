#pragma once

#include "VulkanHelper.h"

#include <filesystem>
#include <chrono>
#include <tuple>
#include <array>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RIGHT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Application
{
public:
    static Application New();

    ~Application() = default;

    void Run();

private:

    void Resize();

    Application(
        VulkanHelper::Instance&& instance,
        VulkanHelper::Window&& window,
        VulkanHelper::Device&& device,
        VulkanHelper::Renderer&& renderer,
        VulkanHelper::Shader&& vertexShader,
        VulkanHelper::Shader&& fragmentShader,
        VulkanHelper::Pipeline&& graphicsPipeline,
        VulkanHelper::Mesh&& loadedMesh,
        VulkanHelper::DescriptorPool&& descriptorPool,
        VulkanHelper::DescriptorSet&& textureSet,
        VulkanHelper::Image&& textureImage,
        VulkanHelper::ImageView&& textureImageView,
        VulkanHelper::Image&& depthImage,
        VulkanHelper::ImageView&& depthImageView,
        VulkanHelper::Image&& colorImage,
        VulkanHelper::ImageView&& colorImageView,
        VulkanHelper::Sampler&& textureSampler,
        VulkanHelper::PushConstant&& pushConstant,
        VulkanHelper::CommandPool&& commandPool,
        VulkanHelper::CommandBuffer&& initializationCmd
    )
    : m_Instance(std::move(instance)),
      m_Window(std::move(window)),
      m_Device(std::move(device)),
      m_Renderer(std::move(renderer)),
      m_LoadedMesh(std::move(loadedMesh)),
      m_DescriptorPool(std::move(descriptorPool)),
      m_TextureSet(std::move(textureSet)),
      m_VertexShader(std::move(vertexShader)),
      m_FragmentShader(std::move(fragmentShader)),
      m_GraphicsPipeline(std::move(graphicsPipeline)),
      m_TextureImage(std::move(textureImage)),
      m_TextureImageView(std::move(textureImageView)),
      m_DepthImage(std::move(depthImage)),
      m_DepthImageView(std::move(depthImageView)),
      m_ColorImage(std::move(colorImage)),
      m_ColorImageView(std::move(colorImageView)),
      m_TextureSampler(std::move(textureSampler)),
      m_PushConstant(std::move(pushConstant)),
      m_CommandPool(std::move(commandPool)),
      m_InitializationCmd(std::move(initializationCmd))
    {}

    VulkanHelper::Instance m_Instance;
    VulkanHelper::Window m_Window;
    VulkanHelper::Device m_Device;
    VulkanHelper::Renderer m_Renderer;

    VulkanHelper::Mesh m_LoadedMesh;

    VulkanHelper::DescriptorPool m_DescriptorPool;
    VulkanHelper::DescriptorSet m_TextureSet;

    VulkanHelper::Shader m_VertexShader;
    VulkanHelper::Shader m_FragmentShader;
    VulkanHelper::Pipeline m_GraphicsPipeline;

    VulkanHelper::Image m_TextureImage;
    VulkanHelper::ImageView m_TextureImageView;

    VulkanHelper::Image m_DepthImage;
    VulkanHelper::ImageView m_DepthImageView;

    VulkanHelper::Image m_ColorImage;
    VulkanHelper::ImageView m_ColorImageView;

    VulkanHelper::Sampler m_TextureSampler;

    VulkanHelper::PushConstant m_PushConstant;

    VulkanHelper::CommandPool m_CommandPool;
    VulkanHelper::CommandBuffer m_InitializationCmd;
};