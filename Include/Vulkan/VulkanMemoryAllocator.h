#pragma once

#include "Core/Error.h"
#include "Core/Expected.h"
#include "Core/UniquePtr.h"

namespace VulkanHelper
{
    class VulkanMemoryAllocator
    {
    public:
        struct Config
        {

        };

        [[nodiscard]] static Expected<VulkanMemoryAllocator, VHResult> New(const Config& config);

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;
    };
}