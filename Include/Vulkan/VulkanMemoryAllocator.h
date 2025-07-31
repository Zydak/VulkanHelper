#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"
#include "Core/Macros.h"

namespace VulkanHelper
{
    class VulkanMemoryAllocator
    {
    public:
        struct Config
        {

        };

        [[nodiscard]] static Expected<VulkanMemoryAllocator, VHResult> New(const Config& config);
    private:
        class Impl;
        UniquePtr<Impl> m_Impl;

        #undef VULKAN_MEMORY_ALLOCATOR_CLASS
        DECLARE_FRIENDS();
        #define VULKAN_MEMORY_ALLOCATOR_CLASS VulkanMemoryAllocator
    };
}