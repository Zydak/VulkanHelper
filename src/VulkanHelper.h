#pragma once

#ifdef VL_ENTRY_POINT
#include "Vulkan-Helper/src/VulkanHelper/core/EntryPoint.h"
#endif

#include "Vulkan-Helper/src/VulkanHelper/core/Application.h"
#include "Vulkan-Helper/src/VulkanHelper/core/Input.h"
#include "Vulkan-Helper/src/VulkanHelper/Scene/Components.h"
#include "Vulkan-Helper/src/VulkanHelper/Scene/Entity.h"
#include "Vulkan-Helper/src/VulkanHelper/Scene/Scene.h"
#include "Vulkan-Helper/src/VulkanHelper/Renderer/Renderer.h"
#include "Vulkan-Helper/src/VulkanHelper/Renderer/FontAtlas.h"
#include "Vulkan-Helper/src/VulkanHelper/Renderer/Text.h"
#include "Vulkan-Helper/src/VulkanHelper/Math/Transform.h"
#include "Vulkan-Helper/src/VulkanHelper/Renderer/AccelerationStructure.h"
#include "Vulkan-Helper/src/VulkanHelper/Renderer/Denoiser.h"
#include "Vulkan-Helper/src/Vulkan/SBT.h"
#include "Vulkan-Helper/src/Vulkan/PushConstant.h"
#include "Vulkan-Helper/src/Vulkan/Shader.h"
#include "Vulkan-Helper/src/Vulkan/DeleteQueue.h"

#include "Vulkan-Helper/src/VulkanHelper/Math/Quaternion.h"
#include "Vulkan-Helper/src/VulkanHelper/Effects/Tonemap.h"
#include "Vulkan-Helper/src/VulkanHelper/Effects/Bloom.h"
#include "Vulkan-Helper/src/VulkanHelper/Effects/Effect.h"
#include "Vulkan-Helper/src/VulkanHelper/Math/Math.h"

#include "Vulkan-Helper/src/VulkanHelper/Asset/AssetManager.h"
#include "Vulkan-Helper/src/VulkanHelper/Asset/Serializer.h"

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
