#pragma once

namespace VulkanHelper
{
    /**
     * @brief Error codes used throughout the VulkanHelper library.
     *
     * VHError enumerates all possible error and status codes that can be returned by VulkanHelper functions,
     * including both generic and Vulkan-specific errors.
     *
     * @note This enum maps closely to Vulkan's VkResult codes, but also includes custom values for library-specific errors.
     */

    enum class VHResult
    {
        OK = 0,                         ///< Operation completed successfully. (VK_SUCCESS)
        NOT_READY = 1,                  ///< A fence or query has not yet completed. (VK_NOT_READY)
        TIMEOUT = 2,                    ///< A wait operation has not completed in the specified time. (VK_TIMEOUT)
        EVENT_SET = 3,                  ///< An event is signaled. (VK_EVENT_SET)
        EVENT_RESET = 4,                ///< An event is unsignaled. (VK_EVENT_RESET)
        INCOMPLETE = 5,                 ///< A return array was too small for the result. (VK_INCOMPLETE)
        OUT_OF_HOST_MEMORY = -1,        ///< A host memory allocation has failed. (VK_ERROR_OUT_OF_HOST_MEMORY)
        OUT_OF_DEVICE_MEMORY = -2,      ///< A device memory allocation has failed. (VK_ERROR_OUT_OF_DEVICE_MEMORY)
        INITIALIZATION_FAILED = -3,     ///< Initialization of an object could not be completed. (VK_ERROR_INITIALIZATION_FAILED)
        DEVICE_LOST = -4,               ///< The logical device has been lost. (VK_ERROR_DEVICE_LOST)
        MEMORY_MAP_FAILED = -5,         ///< Mapping of a memory object has failed. (VK_ERROR_MEMORY_MAP_FAILED)
        LAYER_NOT_PRESENT = -6,         ///< A requested layer is not present or could not be loaded. (VK_ERROR_LAYER_NOT_PRESENT)
        EXTENSION_NOT_PRESENT = -7,     ///< A requested extension is not supported. (VK_ERROR_EXTENSION_NOT_PRESENT)
        FEATURE_NOT_PRESENT = -8,       ///< A requested feature is not supported. (VK_ERROR_FEATURE_NOT_PRESENT)
        INCOMPATIBLE_DRIVER = -9,       ///< The requested version of Vulkan is not supported by the driver. (VK_ERROR_INCOMPATIBLE_DRIVER)
        TOO_MANY_OBJECTS = -10,         ///< Too many objects of the type have already been created. (VK_ERROR_TOO_MANY_OBJECTS)
        FORMAT_NOT_SUPPORTED = -11,     ///< A requested format is not supported on this device. (VK_ERROR_FORMAT_NOT_SUPPORTED)
        FRAGMENTED_POOL = -12,          ///< A pool allocation has failed due to fragmentation of the pool's memory. (VK_ERROR_FRAGMENTED_POOL)
        UNKNOWN = -13,                  ///< An unknown error has occurred. (VK_ERROR_UNKNOWN)
        WRONG_ARGUMENTS = -14,          ///< A function was called with incorrect or invalid arguments.
        
        // Vulkan extensions and platform-specific errors
        OUT_OF_POOL_MEMORY = -1000069000, ///< A pool memory allocation has failed. (VK_ERROR_OUT_OF_POOL_MEMORY)
        INVALID_EXTERNAL_HANDLE = -1000072003, ///< An external handle is not valid. (VK_ERROR_INVALID_EXTERNAL_HANDLE)
        FRAGMENTATION = -1000161000,   ///< A descriptor pool creation has fragmented the pool. (VK_ERROR_FRAGMENTATION)
        INVALID_OPAQUE_CAPTURE_ADDRESS = -1000257000, ///< An invalid opaque capture address was provided. (VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        PIPELINE_COMPILE_REQUIRED = 1000297000, ///< A pipeline compile is required. (VK_PIPELINE_COMPILE_REQUIRED)
        SURFACE_LOST_KHR = -1000000000, ///< A surface is no longer available. (VK_ERROR_SURFACE_LOST_KHR)
        NATIVE_WINDOW_IN_USE_KHR = -1000000001, ///< The requested window is already connected to a VkSurfaceKHR. (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        SUBOPTIMAL_KHR = 1000001003,    ///< A swapchain no longer matches the surface properties exactly, but can still be used. (VK_SUBOPTIMAL_KHR)
        OUT_OF_DATE_KHR = -1000001004,  ///< A surface has changed in such a way that it is no longer compatible with the swapchain. (VK_ERROR_OUT_OF_DATE_KHR)
        INCOMPATIBLE_DISPLAY_KHR = -1000003001, ///< The display used by a swapchain does not use the same presentable image layout. (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        VALIDATION_FAILED_EXT = -1000011001, ///< Validation failed. (VK_ERROR_VALIDATION_FAILED_EXT)
        INVALID_SHADER_NV = -1000012000, ///< Invalid shader. (VK_ERROR_INVALID_SHADER_NV)
        IMAGE_USAGE_NOT_SUPPORTED_KHR = -1000023000, ///< Image usage not supported. (VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
        VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR = -1000023001, ///< Video picture layout not supported. (VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
        VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR = -1000023002, ///< Video profile operation not supported. (VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
        VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR = -1000023003, ///< Video profile format not supported. (VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
        VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR = -1000023004, ///< Video profile codec not supported. (VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
        VIDEO_STD_VERSION_NOT_SUPPORTED_KHR = -1000023005, ///< Video std version not supported. (VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
        INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT = -1000158000, ///< Invalid DRM format modifier plane layout. (VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        NOT_PERMITTED_KHR = -1000174001, ///< Operation not permitted. (VK_ERROR_NOT_PERMITTED_KHR)
        FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT = -1000255000, ///< Full screen exclusive mode lost. (VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        THREAD_IDLE_KHR = 1000268000, ///< Thread is idle. (VK_THREAD_IDLE_KHR)
        THREAD_DONE_KHR = 1000268001, ///< Thread is done. (VK_THREAD_DONE_KHR)
        OPERATION_DEFERRED_KHR = 1000268002, ///< Operation deferred. (VK_OPERATION_DEFERRED_KHR)
        OPERATION_NOT_DEFERRED_KHR = 1000268003, ///< Operation not deferred. (VK_OPERATION_NOT_DEFERRED_KHR)
        COMPRESSION_EXHAUSTED_EXT = -1000338000, ///< Compression exhausted. (VK_ERROR_COMPRESSION_EXHAUSTED_EXT)
    };
}