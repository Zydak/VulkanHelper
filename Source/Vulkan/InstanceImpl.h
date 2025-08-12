#pragma once
#include "Vulkan/Instance.h"

#include <vulkan/vulkan.h>

namespace VulkanHelper
{
    class Instance::Impl
    {
    public:
        [[nodiscard]] static Expected<Impl, VHResult> New(bool addGLFWExtensions);

        Impl(const Impl& other) = delete;
        Impl& operator=(const Impl& other) = delete;

        Impl(Impl&& other) noexcept;
        Impl& operator=(Impl&& other) noexcept;

        ~Impl();

        [[nodiscard]] inline static Impl* GetImplementation(const Instance* publicInterface) { return publicInterface->m_Impl.Get(); }
        [[nodiscard]] inline static Instance CreatePublicInterface(Impl&& impl) { return Instance(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl))))); }
        [[nodiscard]] inline static UniquePtr<Instance> CreatePublicInterfacePtr(Impl&& impl) { return UniquePtr(new Instance(VulkanHelper::Move(UniquePtr<Impl>(new Impl(Move(impl)))))); }

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