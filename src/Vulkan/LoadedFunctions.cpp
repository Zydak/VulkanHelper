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