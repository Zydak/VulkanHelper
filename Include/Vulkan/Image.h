#pragma once
#include "Core/Expected.h"
#include "Core/Macros.h"
#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Enums.h"

namespace VulkanHelper
{
    class Device;
    class CommandBuffer;

    class Image
    {
    public:
        enum class Usage
        {
            TRANSFER_SRC_BIT = 0x00000001,
            TRANSFER_DST_BIT = 0x00000002,
            SAMPLED_BIT = 0x00000004,
            STORAGE_BIT = 0x00000008,
            COLOR_ATTACHMENT_BIT = 0x00000010,
            DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
            TRANSIENT_ATTACHMENT_BIT = 0x00000040,
            INPUT_ATTACHMENT_BIT = 0x00000080,
            UNDEFINED = 0x7FFFFFFF
        };

        enum class Aspect
        {
            NONE = 0,
            COLOR_BIT = 0x00000001,
            DEPTH_BIT = 0x00000002,
            STENCIL_BIT = 0x00000004,
            METADATA_BIT = 0x00000008,
            PLANE_0_BIT = 0x00000010,
            PLANE_1_BIT = 0x00000020,
            PLANE_2_BIT = 0x00000040,
            UNDEFINED = 0x7FFFFFFF
        };

        enum class Tiling
        {
            OPTIMAL = 0,
            LINEAR = 1,
            UNDEFINED = 0x7FFFFFFF
        };

        enum class Layout
        {
            UNDEFINED = 0,
            GENERAL = 1,
            COLOR_ATTACHMENT_OPTIMAL = 2,
            DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
            DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
            SHADER_READ_ONLY_OPTIMAL = 5,
            TRANSFER_SRC_OPTIMAL = 6,
            TRANSFER_DST_OPTIMAL = 7,
            PREINITIALIZED = 8,
            DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
            DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
            DEPTH_ATTACHMENT_OPTIMAL = 1000241000,
            DEPTH_READ_ONLY_OPTIMAL = 1000241001,
            STENCIL_ATTACHMENT_OPTIMAL = 1000241002,
            STENCIL_READ_ONLY_OPTIMAL = 1000241003,
            READ_ONLY_OPTIMAL = 1000314000,
            ATTACHMENT_OPTIMAL = 1000314001,
            PRESENT_SRC_KHR = 1000001002,
        };

        struct Config
        {
            VulkanHelper::Device* Device = nullptr;

            uint32_t Height = 0;
            uint32_t Width = 0;
            uint32_t MipCount = 1;
            uint32_t LayerCount = 1;

            VulkanHelper::Format Format = Format::UNDEFINED;
            VulkanHelper::Image::Usage Usage = Usage::UNDEFINED;
            VulkanHelper::MemoryProperties MemoryProperties = MemoryProperties::UNDEFINED;
            VulkanHelper::Image::Tiling Tiling = Tiling::OPTIMAL;
            VulkanHelper::Image::Aspect Aspect = Aspect::COLOR_BIT;
        };

        [[nodiscard]] static Expected<Image, VHResult> New(const Config& config);

        Image(const Image& other) = delete;
        Image& operator=(const Image& other) = delete;
        
        Image(Image&& other) noexcept;
        Image& operator=(Image&& other) noexcept;

        ~Image();

        void TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount);

        [[nodiscard]] Format GetFormat() const;
        [[nodiscard]] Layout GetLayout() const;
        [[nodiscard]] MemoryProperties GetMemoryProperties() const;
        [[nodiscard]] Aspect GetAspect() const;
        
        [[nodiscard]] uint32_t GetWidth() const;
        [[nodiscard]] uint32_t GetHeight() const;
        [[nodiscard]] uint32_t GetLayerCount() const;
        [[nodiscard]] uint32_t GetMipCount() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Image(UniquePtr<Impl> && impl);

        friend class Swapchain; // Allow Swapchain to construct Image instances
    };

    DEFINE_BITWISE_OPERATORS(Image::Usage)
    DEFINE_BITWISE_OPERATORS(Image::Aspect)
}