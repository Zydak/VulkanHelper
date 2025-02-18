#pragma once
#include "Pch.h"

#include "vulkan/vulkan.h"

namespace VulkanHelper
{
	VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo);

}