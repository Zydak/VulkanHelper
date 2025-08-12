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
        VulkanHelper::Shader&& raygenShader,
        VulkanHelper::Shader&& hitShader,
        VulkanHelper::Shader&& missShader,
        VulkanHelper::Pipeline&& rtPipeline,
        VulkanHelper::Mesh&& loadedMesh,
        VulkanHelper::DescriptorPool&& descriptorPool,
        VulkanHelper::DescriptorSet&& textureSet,
        VulkanHelper::Image&& depthImage,
        VulkanHelper::ImageView&& depthImageView,
        VulkanHelper::Image&& colorImage,
        VulkanHelper::ImageView&& colorImageView,
        VulkanHelper::Buffer&& uniformBufferCamera,
        VulkanHelper::CommandPool&& commandPool,
        VulkanHelper::CommandBuffer&& initializationCmd,
        VulkanHelper::BLAS&& blas,
        VulkanHelper::TLAS&& tlas
    )
    : m_Instance(std::move(instance)),
      m_Window(std::move(window)),
      m_Device(std::move(device)),
      m_Renderer(std::move(renderer)),
      m_LoadedMesh(std::move(loadedMesh)),
      m_DescriptorPool(std::move(descriptorPool)),
      m_TextureSet(std::move(textureSet)),
      m_RaygenShader(std::move(raygenShader)),
      m_HitShader(std::move(hitShader)),
      m_MissShader(std::move(missShader)),
      m_RTPipeline(std::move(rtPipeline)),
      m_DepthImage(std::move(depthImage)),
      m_DepthImageView(std::move(depthImageView)),
      m_ColorImage(std::move(colorImage)),
      m_ColorImageView(std::move(colorImageView)),
      m_UniformBufferCamera(std::move(uniformBufferCamera)),
      m_CommandPool(std::move(commandPool)),
      m_InitializationCmd(std::move(initializationCmd)),
      m_BLAS(std::move(blas)),
      m_TLAS(std::move(tlas))
    {}

    VulkanHelper::Instance m_Instance;
    VulkanHelper::Window m_Window;
    VulkanHelper::Device m_Device;
    VulkanHelper::Renderer m_Renderer;

    VulkanHelper::Mesh m_LoadedMesh;

    VulkanHelper::DescriptorPool m_DescriptorPool;
    VulkanHelper::DescriptorSet m_TextureSet;

    VulkanHelper::Shader m_RaygenShader;
    VulkanHelper::Shader m_HitShader;
    VulkanHelper::Shader m_MissShader;
    VulkanHelper::Pipeline m_RTPipeline;

    VulkanHelper::Image m_DepthImage;
    VulkanHelper::ImageView m_DepthImageView;

    VulkanHelper::Image m_ColorImage;
    VulkanHelper::ImageView m_ColorImageView;

    VulkanHelper::Buffer m_UniformBufferCamera;

    VulkanHelper::CommandPool m_CommandPool;
    VulkanHelper::CommandBuffer m_InitializationCmd;

    VulkanHelper::BLAS m_BLAS;
    VulkanHelper::TLAS m_TLAS;
};