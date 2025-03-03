#include "Pch.h"
#include "LoadedFunctions.h"

#include "Instance.h"
#include "Logger/Logger.h"

VkResult VulkanHelper::vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
	static auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(VulkanHelper::Instance::Get()->GetHandle(), "vkSetDebugUtilsObjectNameEXT");
	if (func != nullptr) { return func(device, pNameInfo); }
	else { VH_CHECK(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
}

VkResult VulkanHelper::vkCreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
	static auto func = (PFN_vkCreateRayTracingPipelinesKHR)vkGetInstanceProcAddr(VulkanHelper::Instance::Get()->GetHandle(), "vkCreateRayTracingPipelinesKHR");
	if (func != nullptr) { return func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines); }
	else { VH_CHECK(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return VK_RESULT_MAX_ENUM; }
}

void VulkanHelper::vkCmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo* pRenderingInfo)
{
	static auto func = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(VulkanHelper::Instance::Get()->GetHandle(), "vkCmdBeginRenderingKHR");
	if (func != nullptr) { return func(commandBuffer, pRenderingInfo); }
	else { VH_CHECK(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return; }
}

void VulkanHelper::vkCmdEndRendering(VkCommandBuffer commandBuffer)
{
	static auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(VulkanHelper::Instance::Get()->GetHandle(), "vkCmdEndRenderingKHR");
	if (func != nullptr) { return func(commandBuffer); }
	else { VH_CHECK(false, "VK_ERROR_EXTENSION_NOT_PRESENT"); return; }
}
