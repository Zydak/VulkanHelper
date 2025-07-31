#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Vector.h"
#include "Core/Macros.h"

typedef struct VkInstance_T* VkInstance;

namespace VulkanHelper
{
    class PhysicalDevice;

    /**
     * @class Instance
     * @brief RAII wrapper for a Vulkan instance.
     *
     * Manages the lifetime of a Vulkan instance, providing functionality to create and query physical devices.
     */
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
         * @return VulkanHelper::Expected<Instance, VHError> An expected containing the created Instance on success, or a VHError on failure.
         */
        static VulkanHelper::Expected<Instance, VHResult> New(const Config& config);

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
         * @param deviceExtensions A vector of required Vulkan Device Extension names.
         * @return VulkanHelper::Vector<PhysicalDevice> A vector of suitable PhysicalDevice objects.
         */
        VulkanHelper::Vector<PhysicalDevice> GetSuitablePhysicalDevices(const VulkanHelper::Vector<const char*>& deviceExtensions) const;

        /**
         * @brief Retrieves the underlying Vulkan VkInstance handle.
         *
         * @return VkInstance The Vulkan instance handle managed by this object.
         */
        [[nodiscard]] VkInstance GetInstance() const;

        ~Instance();

    private:
        class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Instance(VulkanHelper::UniquePtr<Impl>&& impl);

        #undef INSTANCE_CLASS
        DECLARE_FRIENDS();
        #define INSTANCE_CLASS Instance
    };
}