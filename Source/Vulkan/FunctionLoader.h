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

        static inline VkResult vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
        {
            static auto func = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(m_Instance, "vkCreateRayTracingPipelinesKHR");
            VH_ASSERT(func != nullptr, "vkCreateRayTracingPipelinesKHR not Present!");
            return func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
        }

        static inline void vkCmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth)
        {
            static auto func = (PFN_vkCmdTraceRaysKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdTraceRaysKHR");
            VH_ASSERT(func != nullptr, "vkCmdTraceRaysKHR not Present!");
            return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
        }

    private:
        inline static VkInstance m_Instance;
    };
}