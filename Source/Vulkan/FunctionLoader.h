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

        static inline VkResult vkCreateAccelerationStructureKHR(VkDevice device, VkAccelerationStructureCreateInfoKHR* createInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* structure)
        {
            static auto func = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(m_Instance, "vkCreateAccelerationStructureKHR");
            VH_ASSERT(func != nullptr, "vkCreateAccelerationStructureKHR not Present!");
            return func(device, createInfo, pAllocator, structure);
        }

        static inline void vkDestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR structure, const VkAllocationCallbacks* pAllocator)
        {
            static auto func = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(m_Instance, "vkDestroyAccelerationStructureKHR");
            VH_ASSERT(func != nullptr, "vkDestroyAccelerationStructureKHR not Present!");
            return func(device, structure, pAllocator);
        }

        static inline void vkCmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos)
        {
            static auto func = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdBuildAccelerationStructuresKHR");
            VH_ASSERT(func != nullptr, "vkCmdBuildAccelerationStructuresKHR not Present!");
            return func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
        }

        static inline void vkCmdWriteAccelerationStructuresPropertiesKHR(VkCommandBuffer commandBuffer, uint32_t accelerationStructureCount, const VkAccelerationStructureKHR* pAccelerationStructures, VkQueryType queryType, VkQueryPool queryPool, uint32_t firstQuery)
        {
            static auto func = (PFN_vkCmdWriteAccelerationStructuresPropertiesKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdWriteAccelerationStructuresPropertiesKHR");
            VH_ASSERT(func != nullptr, "vkCmdWriteAccelerationStructuresPropertiesKHR not Present!");
            return func(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
        }

        static inline void vkCmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR* pInfo)
        {
            static auto func = (PFN_vkCmdCopyAccelerationStructureKHR)vkGetInstanceProcAddr(m_Instance, "vkCmdCopyAccelerationStructureKHR");
            VH_ASSERT(func != nullptr, "vkCmdCopyAccelerationStructureKHR not Present!");
            return func(commandBuffer, pInfo);
        }

        static inline void vkGetAccelerationStructureBuildSizesKHR(VkDevice device, VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo, const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo)
        {
            static auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(m_Instance, "vkGetAccelerationStructureBuildSizesKHR");
            VH_ASSERT(func != nullptr, "vkGetAccelerationStructureBuildSizesKHR not Present!");
            return func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
        }

        static inline VkDeviceAddress vkGetAccelerationStructureDeviceAddressKHR(VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
        {
            static auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(m_Instance, "vkGetAccelerationStructureDeviceAddressKHR");
            VH_ASSERT(func != nullptr, "vkGetAccelerationStructureDeviceAddressKHR not Present!");
            return func(device, pInfo);
        }

    private:
        inline static VkInstance m_Instance;
    };
}