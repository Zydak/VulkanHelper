#pragma once
#include "Vulkan/Instance.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Instance::Impl
    {
    public:
        [[nodiscard]] static Expected<SharedPtr<Impl>, VHResult> New(bool addGLFWExtensions);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static SharedPtr<Impl> GetImplementation(const Instance& publicInterface) { return publicInterface.m_Impl; }
        [[nodiscard]] inline static Instance CreatePublicInterface(const SharedPtr<Impl>& impl) { return Instance(impl); }

        Vector<PhysicalDevice> GetSuitablePhysicalDevices() const;

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