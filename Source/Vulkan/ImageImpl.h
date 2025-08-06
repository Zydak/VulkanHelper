#pragma once
#include "Core/Enums.h"
#include "Vulkan/Image.h"
#include "Vulkan/Swapchain.h"

#include "DeviceImpl.h"
#include "VulkanMemoryAllocator.h"

typedef struct VkImage_T* VkImage;

namespace VulkanHelper
{
    class Image::Impl
    {
    public:
        [[nodiscard]] static Expected<UniquePtr<Impl>, VHResult> New(Device::Impl* device, uint32_t height, uint32_t width, uint32_t mipCount, uint32_t layerCount, Format format, Usage usage, Tiling tiling, Aspect aspect, Layout initialLayout, SampleCount sampleCount, bool usePersistentStagingBuffer, bool allowMapping);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Image* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Image CreatePublicInterface(UniquePtr<Impl>&& impl) { return Image(VulkanHelper::Move(impl)); }

        [[nodiscard]] inline Device::Impl* GetDevice() const { return m_Device; }

        [[nodiscard]] inline Format GetFormat() const { return m_Format; }
        [[nodiscard]] inline Layout GetLayout() const { return m_Layout; }
        [[nodiscard]] inline Aspect GetAspect() const { return m_Aspect; }
        
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Width; }
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Height; }
        [[nodiscard]] inline uint32_t GetLayerCount() const { return m_LayerCount; }
        [[nodiscard]] inline uint32_t GetMipCount() const { return m_MipCount; }

        [[nodiscard]] inline VkImage GetImage() const { return m_Allocation.image; }

        void TransitionImageLayout(Layout newLayout, CommandBuffer& commandBuffer, uint32_t baseLayer, uint32_t layerCount);

        [[nodiscard]] Expected<void*, VHResult> Map();
        void Unmap();
        VHResult UploadData(const void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd = nullptr);

        VHResult DownloadData(void* data, uint64_t size, uint64_t offset, CommandBuffer* cmd = nullptr) const;

    private:
        Device::Impl* m_Device;

        Format m_Format;
        Layout m_Layout;
        Aspect m_Aspect;

        uint32_t m_Width;
        uint32_t m_Height;
        uint32_t m_LayerCount;
        uint32_t m_MipCount;

        bool m_Mapable;

        VulkanMemoryAllocator::ImageAllocation m_Allocation;
        VulkanMemoryAllocator::BufferAllocation m_StagingBufferAllocation;

        explicit Impl(Device::Impl* device,
            Format format,
            Layout layout,
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
            , m_Layout(layout)
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
        explicit Impl(Device::Impl* device,
            Format format,
            Layout layout,
            Aspect aspect,
            uint32_t width,
            uint32_t height,
            uint32_t layerCount,
            uint32_t mipCount,
            VkImage image
        )
            : m_Device(device)
            , m_Format(format)
            , m_Layout(layout)
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