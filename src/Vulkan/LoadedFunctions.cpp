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
