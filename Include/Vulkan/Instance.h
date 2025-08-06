#pragma once

#include "Core/Expected.h"
#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Vector.h"

namespace VulkanHelper
{
    class PhysicalDevice;

    /**
     * @class Instance
     * @brief RAII wrapper for a Vulkan instance. Manages the lifetime and functionality of a VkInstance object.
     */
    class Instance
    {
    public:
        /**
         * @brief Configuration options for Instance creation
         */
        struct Config
        {
            /**
             * @brief When true, adds GLFW-required instance extensions to the creation info
             */
            bool AddGLFWExtensions = false;
        };

        /**
         * @brief Creates a new Vulkan instance with the specified configuration.
         * 
         * @param config Configuration options for instance creation
         * @return VulkanHelper::Expected<Instance, VHResult> The created Instance or error code
         * @note Requires valid Vulkan installation
         */
        static VulkanHelper::Expected<Instance, VHResult> New(const Config& config);

        Instance(const Instance& other) = delete;
        Instance& operator=(const Instance& other) = delete;

        Instance(Instance&& other) noexcept;
        Instance& operator=(Instance&& other) noexcept;

        /**
         * @brief Enumerates all physical devices that meet Vulkan requirements. Returns a vector of suitable PhysicalDevice objects.
         * 
         * @return VulkanHelper::Vector<PhysicalDevice> List of suitable physical devices
         */
        VulkanHelper::Vector<PhysicalDevice> GetSuitablePhysicalDevices() const;

        ~Instance();

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Instance(VulkanHelper::UniquePtr<Impl>&& impl);
    };
}