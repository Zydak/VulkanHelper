#pragma once
#include <expected>
#include "Core/Error.h"
#include "Vulkan/PhysicalDevice.h"

// Forward declare stuff since client application isn't linking to Vulkan
struct VkDebugUtilsMessengerCreateInfoEXT;
typedef struct VkInstance_T* VkInstance;
typedef struct VkDebugUtilsMessengerEXT_T* VkDebugUtilsMessengerEXT;

namespace VulkanHelper
{
    class Instance
    {
    public:
        struct Config
        {
            bool AddGLFWExtensions = false;
        };

        /**
         * @brief Creates a new Vulkan instance with the specified configuration.
         *
         * This static factory function attempts to create a Vulkan instance according to the provided configuration.
         * If successful, it returns an Instance object; otherwise, it returns a VHError describing the failure.
         *
         * @param config The configuration struct specifying instance creation options, such as whether to add GLFW extensions.
         * @return std::expected<Instance, VHError> An expected containing the created Instance on success, or a VHError on failure.
         */
        static std::expected<Instance, VHResult> New(const Config& config);

        Instance(const Instance& other) = delete;
        Instance& operator=(const Instance& other) = delete;

        Instance(Instance&& other) noexcept;
        Instance& operator=(Instance&& other) noexcept;

        /**
         * @brief Finds all suitable physical devices for the given extension requirements.
         *
         * This function enumerates all available Vulkan physical devices and checks if they support the required extensions.
         * Only devices that meet the requirements are returned in the result vector.
         *
         * @param extensions A vector of required Vulkan extension names.
         * @return std::vector<PhysicalDevice> A vector of suitable PhysicalDevice objects.
         */
        std::vector<PhysicalDevice> GetSuitablePhysicalDevices(const std::vector<const char*>& extensions) const;

        /**
         * @brief Retrieves the underlying Vulkan VkInstance handle.
         *
         * @return VkInstance The Vulkan instance handle managed by this object.
         */
        [[nodiscard]] inline VkInstance GetInstance() const { return m_Instance; }

        ~Instance();

    private:
        Instance(VkDebugUtilsMessengerEXT messenger, VkInstance instance)
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