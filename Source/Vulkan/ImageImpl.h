#pragma once
#include "Core/Enums.h"
#include "Vulkan/Image.h"
#include "Vulkan/Swapchain.h"

#include "DeviceImpl.h"

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

        [[nodiscard]] inline static Impl* GetImplementation(const Image* publicInterface) { return publicInterface->m_Impl.Get(); }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }

        [[nodiscard]] inline Format GetFormat() const { return m_Format; }
        [[nodiscard]] inline Layout GetLayout() const { return m_Layout; }
        [[nodiscard]] inline MemoryProperties GetMemoryProperties() const { return m_MemoryProperties; }
        [[nodiscard]] inline Aspect GetAspect() const { return m_Aspect; }
        
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Height; }
        [[nodiscard]] inline uint32_t GetLayerCount() const { return m_LayerCount; }
        [[nodiscard]] inline uint32_t GetMipCount() const { return m_MipCount; }

        [[nodiscard]] inline VkImage GetImage() const { return m_Image; }

        void TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount);
    private:

        Device::Impl* m_Device;

        Format m_Format;
        Layout m_Layout;
        MemoryProperties m_MemoryProperties;
        Aspect m_Aspect;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_LayerCount;
        uint32_t m_MipCount;

        VkImage m_Image;

        Impl(Device::Impl* device,
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