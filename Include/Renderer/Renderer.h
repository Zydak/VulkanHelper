#pragma once

#include "Core/UniquePtr.h"
#include "Core/Error.h"
#include "Core/Expected.h"

#include "Vulkan/CommandBuffer.h"

namespace VulkanHelper
{
    class Window;
    class Device;

    class Renderer
    {
    public:
        struct Config
        {
            VulkanHelper::Device* Device = nullptr;
            VulkanHelper::Window* Window = nullptr;
            uint32_t FramesInFlight = 1; // Set to 2 for double buffering, 3 for tripple buffering, and so on.
        };

        [[nodiscard]] static Expected<Renderer, VHResult> New(const Config& config);

        ~Renderer();

        Renderer(const Renderer& other) = delete;
        Renderer& operator=(const Renderer& other) = delete;

        Renderer(Renderer&& other) noexcept;
        Renderer& operator=(Renderer&& other) noexcept;

        [[nodiscard]] Expected<CommandBuffer*, VHResult> BeginFrame();
        [[nodiscard]] VHResult EndFrame();

    private:
        class Impl;
        UniquePtr<Impl> m_Impl;

        Renderer(UniquePtr<Impl>&& impl);
    };
}