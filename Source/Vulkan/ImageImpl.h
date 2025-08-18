#pragma once
#include "Core/Enums.h"
#include "Vulkan/Image.h"
#include "Vulkan/Swapchain.h"

#include "DeviceImpl.h"
#include "CommandBufferImpl.h"
#include "VulkanMemoryAllocator.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Image::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(
            const SharedPtr<Device::Impl>& device,
            uint32_t height,
            uint32_t width,
            uint32_t mipCount,
            uint32_t layerCount,
            Format format,
            Usage usage,
            Tiling tiling,
            Aspect aspect,
            Layout initialLayout,
            SampleCount sampleCount,
            bool usePersistentStagingBuffer,
            bool allowMapping
        );

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Image& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Image CreatePublicInterface(const SharedPtr<Impl>& impl) { return Image(impl); }

        [[nodiscard]] inline SharedPtr<Device::Impl> GetDevice() const { return m_Device; }

        [[nodiscard]] inline Format GetFormat() const { return m_Format; }
        [[nodiscard]] inline Layout GetLayout(uint32_t layer = 0) const { return m_Layout[layer]; }
        [[nodiscard]] inline Aspect GetAspect() const { return m_Aspect; }
        
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Height; }
        [[nodiscard]] inline uint32_t GetLayerCount() const { return m_LayerCount; }
        [[nodiscard]] inline uint32_t GetMipCount() const { return m_MipCount; }
        [[nodiscard]] uint64_t GetSizeInBytes() const;

        [[nodiscard]] inline VkImage GetImage() const { return m_Allocation.image; }

        void TransitionImageLayout(Layout newLayout, const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t baseLayer, uint32_t layerCount);

        [[nodiscard]] VHResult CopyFromImage(const Image& srcImage, const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t srcBaseLayer = 0, uint32_t dstBaseLayer = 0, uint32_t layerCount = 1);
        [[nodiscard]] VHResult BlitFromImage(const Image& srcImage, const SharedPtr<CommandBuffer::Impl> commandBuffer, uint32_t srcBaseLayer = 0, uint32_t dstBaseLayer = 0, uint32_t layerCount = 1);

        [[nodiscard]] Expected<void*, VHResult> Map();
        void Unmap();
        [[nodiscard]] VHResult UploadData(const void* data, uint64_t size, uint64_t offset, const SharedPtr<CommandBuffer::Impl> cmd = nullptr, uint32_t baseLayer = 0);

        [[nodiscard]] VHResult DownloadData(void* data, uint64_t size, uint64_t offset, const SharedPtr<CommandBuffer::Impl> cmd = nullptr) const;

    private:
        SharedPtr<Device::Impl> m_Device;

        Format m_Format;
        Vector<Layout> m_Layout; // Each layer has it's own layout
        Aspect m_Aspect;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_LayerCount;
        uint32_t m_MipCount;

        bool m_Mapable;

        VulkanMemoryAllocator::ImageAllocation m_Allocation;
        VulkanMemoryAllocator::BufferAllocation m_StagingBufferAllocation;

        explicit Impl(
            const SharedPtr<Device::Impl>& device,
            Format format,
            const Vector<Layout>& layout,
            Aspect aspect,
            uint32_t width,
            uint32_t height,
            uint32_t layerCount,
            uint32_t mipCount,
            bool mapable,
            VulkanMemoryAllocator::ImageAllocation&& allocation,
            VulkanMemoryAllocator::BufferAllocation&& stagingBuffer
        )
            : m_Device(device)
            , m_Format(format)
            , m_Layout(layout.Clone())
            , m_Aspect(aspect)
            , m_Width(width)
            , m_Height(height)
            , m_LayerCount(layerCount)
            , m_MipCount(mipCount)
            , m_Mapable(mapable)
            , m_Allocation(Move(allocation))
            , m_StagingBufferAllocation(Move(stagingBuffer))
        {}

        // Special case, alternative constructor for swapchain
        explicit Impl(
            const SharedPtr<Device::Impl>& device,
            Format format,
            const Vector<Layout>& layout,
            Aspect aspect,
            uint32_t width,
            uint32_t height,
            uint32_t layerCount,
            uint32_t mipCount,
            VkImage image
        )
            : m_Device(device)
            , m_Format(format)
            , m_Layout(layout.Clone())
            , m_Aspect(aspect)
            , m_Width(width)
            , m_Height(height)
            , m_LayerCount(layerCount)
            , m_MipCount(mipCount)
            , m_Allocation({image, nullptr})
        {}

        // Swapchain needs to be able to construct image from raw VkImage handle given by the VkSwapchain
        friend class Swapchain::Impl;
    };
}