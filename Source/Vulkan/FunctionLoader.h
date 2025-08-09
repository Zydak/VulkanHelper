#pragma once

#include <vulkan/vulkan.h>
#include "Log/Log.h"

namespace VulkanHelper
{
    class FunctionLoader
    {
    public:
        static void SetInstance(VkInstance instance) { m_Instance = instance; }

        static inline void vkCmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
        {
            static auto func = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdBeginRenderingKHR");
            VH_ASSERT(func != nullptr, "vkCmdBeginRendering Not Present!");
            return func(commandBuffer, pRenderingInfo);
        }

        static inline void vkCmdEndRendering(VkCommandBuffer commandBuffer)
        {
            static auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdEndRenderingKHR");
            VH_ASSERT(func != nullptr, "vkCmdEndRendering Not Present!");
            return func(commandBuffer);
        }

        static inline VkResult vkGetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData)
        {
            static auto func = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(m_Instance, "vkGetRayTracingShaderGroupHandlesKHR");
            VH_ASSERT(func != nullptr, "vkGetRayTracingShaderGroupHandlesKHR not Present!");
            return func(device, pipeline, firstGroup, groupCount, dataSize, pData);
        }

    private:
        inline static VkInstance m_Instance;
    };
}