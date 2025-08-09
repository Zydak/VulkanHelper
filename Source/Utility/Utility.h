#pragma once

#include <cstdint>
#include "Core/Enums.h"
#include "Log/Log.h"

namespace VulkanHelper
{
    uint32_t GetFormatSize(VulkanHelper::Format format);

    template<typename T>
    static uint64_t GetAlignment(T value, uint64_t alignment)
    {
        return (static_cast<uint64_t>(value) + alignment - 1) & ~(alignment - 1);
    }
}