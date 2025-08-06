#pragma once

#include <cstdint>
#include "Core/Enums.h"
#include "Log/Log.h"

namespace VulkanHelper
{
    static uint32_t GetFormatSize(VulkanHelper::Format format)
    {
        switch (format)
        {
            case Format::R8_UNORM:
            case Format::R8_SNORM:
            case Format::R8_USCALED:
            case Format::R8_SSCALED:
            case Format::R8_UINT:
            case Format::R8_SINT:
            case Format::R8_SRGB:
                return 1; // 1 byte

            case Format::R8G8_UNORM:
            case Format::R8G8_SNORM:
            case Format::R8G8_USCALED:
            case Format::R8G8_SSCALED:
            case Format::R8G8_UINT:
            case Format::R8G8_SINT:
            case Format::R8G8_SRGB:
                return 2; // 2 bytes

            case Format::R8G8B8_UNORM:
            case Format::R8G8B8_SNORM:
            case Format::R8G8B8_USCALED:
            case Format::R8G8B8_SSCALED:
            case Format::R8G8B8_UINT:
            case Format::R8G8B8_SINT:
            case Format::R8G8B8_SRGB:
                return 3; // 3 bytes

            case Format::R8G8B8A8_UNORM:
            case Format::R8G8B8A8_SNORM:
            case Format::R8G8B8A8_USCALED:
            case Format::R8G8B8A8_SSCALED:
            case Format::R8G8B8A8_UINT:
            case Format::R8G8B8A8_SINT:
            case Format::R8G8B8A8_SRGB:
                return 4; // 4 bytes

            case Format::R16_UNORM:
            case Format::R16_SNORM:
            case Format::R16_USCALED:
            case Format::R16_SSCALED:
            case Format::R16_UINT:
            case Format::R16_SINT:
            case Format::R16_SFLOAT:
                return 2; // 2 bytes

            case Format::R16G16_UNORM:
            case Format::R16G16_SNORM:
            case Format::R16G16_USCALED:
            case Format::R16G16_SSCALED:
            case Format::R16G16_UINT:
            case Format::R16G16_SINT:
            case Format::R16G16_SFLOAT:
                return 4; // 4 bytes

            case Format::R16G16B16_UNORM:
            case Format::R16G16B16_SNORM:
            case Format::R16G16B16_USCALED:
            case Format::R16G16B16_SSCALED:
            case Format::R16G16B16_UINT:
            case Format::R16G16B16_SINT:
            case Format::R16G16B16_SFLOAT:
                return 6; // 6 bytes

            case Format::R16G16B16A16_UNORM:
            case Format::R16G16B16A16_SNORM:
            case Format::R16G16B16A16_USCALED:
            case Format::R16G16B16A16_SSCALED:
            case Format::R16G16B16A16_UINT:
            case Format::R16G16B16A16_SINT:
            case Format::R16G16B16A16_SFLOAT:
                return 8; // 8 bytes

            case Format::R32_UINT:
            case Format::R32_SINT:
            case Format::R32_SFLOAT:
                return 4; // 4 bytes

            case Format::R32G32_UINT:
            case Format::R32G32_SINT:
            case Format::R32G32_SFLOAT:
                return 8; // 8 bytes

            case Format::R32G32B32_UINT:
            case Format::R32G32B32_SINT:
            case Format::R32G32B32_SFLOAT:
                return 12; // 12 bytes

            case Format::R32G32B32A32_UINT:
            case Format::R32G32B32A32_SINT:
            case Format::R32G32B32A32_SFLOAT:
                return 16; // 16 bytes

            case Format::R64_UINT:
            case Format::R64_SINT:
            case Format::R64_SFLOAT:
                return 8; // 8 bytes

            case Format::R64G64_UINT:
            case Format::R64G64_SINT:
            case Format::R64G64_SFLOAT:
                return 16; // 16 bytes

            case Format::R64G64B64_UINT:
            case Format::R64G64B64_SINT:
            case Format::R64G64B64_SFLOAT:
                return 24; // 24 bytes

            case Format::R64G64B64A64_UINT:
            case Format::R64G64B64A64_SINT:
            case Format::R64G64B64A64_SFLOAT:
                return 32; // 32 bytes
        default:
            VH_ASSERT(false, "Unknown format size");
            return 0; // Unknown format
            break;
        }
    }
}