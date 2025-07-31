#pragma once
#include "Core/Enums.h"
#include "Vulkan/Image.h"
#include "Vulkan/Swapchain.h"

typedef struct VkImage_T* VkImage;

namespace VulkanHelper
{
    class Image::Impl
    {
    public:
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(const Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        void TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount);
    private:

        Device* m_Device;
        Format m_Format;
        Layout m_Layout;
        MemoryProperties m_MemoryProperties;
        Aspect m_Aspect;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_LayerCount;
        uint32_t m_MipCount;

        VkImage m_Image;

        Impl(Device* device,
            Format format,
            Layout layout,
            MemoryProperties memoryProp,
            Aspect aspect,
            uint32_t width,
            uint32_t height,
            uint32_t layoutCount,
            uint32_t mipCount,
            VkImage image
        )
            : m_Device(device)
            , m_Format(format)
            , m_Layout(layout)
            , m_MemoryProperties(memoryProp)
            , m_Aspect(aspect)
            , m_Width(width)
            , m_Height(height)
            , m_LayerCount(layoutCount)
            , m_MipCount(mipCount)
            , m_Image(image)
        {}

        // Swapchain needs to be able to construct image from raw VkImage handle given by the VkSwapchain
        friend class Swapchain::Impl;
    };
}