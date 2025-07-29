#pragma once
#include "Vulkan/Instance.h"

struct VkDebugUtilsMessengerCreateInfoEXT;
typedef struct VkDebugUtilsMessengerEXT_T* VkDebugUtilsMessengerEXT;
typedef struct VkInstance_T* VkInstance;

namespace VulkanHelper
{
    class Instance::Impl
    {
    public:
        static VulkanHelper::Expected<VulkanHelper::UniquePtr<Impl>, VHResult> New(const Config& config);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        VulkanHelper::Vector<PhysicalDevice> GetSuitablePhysicalDevices(const VulkanHelper::Vector<const char*>& extensions) const;

        [[nodiscard]] inline VkInstance GetInstance() const { return m_Instance; }
    private:
        Impl(VkDebugUtilsMessengerEXT messenger, VkInstance instance)
            : m_DebugMessenger(messenger),
            m_Instance(instance)
        {}

        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkInstance m_Instance;

        // Dynamically Loaded functions
        static void CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, VkDebugUtilsMessengerEXT* outDebugMessenger);
        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT* debugMessenger);
    };
}