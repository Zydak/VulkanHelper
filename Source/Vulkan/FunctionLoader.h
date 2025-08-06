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
            if (func != nullptr) { return func(commandBuffer, pRenderingInfo); }
            else { VH_LOG_FATAL("vkCmdBeginRendering Not Present!"); return; }
        }

        static inline void vkCmdEndRendering(VkCommandBuffer commandBuffer)
        {
            static auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdEndRenderingKHR");
            if (func != nullptr) { return func(commandBuffer); }
            else { VH_LOG_FATAL("vkCmdEndRendering Not Present!"); return; }
        } 

    private:
        inline static VkInstance m_Instance;
    };
}