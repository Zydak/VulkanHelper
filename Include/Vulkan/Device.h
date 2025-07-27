#pragma once
#include <expected>

#include "Core/Error.h"
#include "Vulkan/PhysicalDevice.h"
#include "Window/Window.h"

typedef struct VkDevice_T* VkDevice;
typedef struct VkQueue_T* VkQueue;
typedef struct VkCommandPool_T* VkCommandPool;

namespace VulkanHelper
{
    class Device
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a Device instance.
         *
         * Specify the physical device to use and, optionally, a window for presentation support.
         * Pass this struct to Device::New() to create a logical device and associated resources.
         */
        struct Config
        {
            /**
             * @brief The Vulkan physical device to use for logical device creation.
             *
             * This must be a valid and suitable physical device selected by the application.
             */
            VulkanHelper::PhysicalDevice PhysicalDevice;
            /**
             * @brief Optional pointer to a Window for presentation support.
             *
             * If non-null, the device will be created with presentation capabilities for the given window's surface.
             * If null, presentation support is not requested.
             */
            const VulkanHelper::Window* Window = nullptr;
        };

        /**
         * @brief Creates a new logical Vulkan device and associated queues/command pools.
         *
         * This static factory function attempts to create a logical device from the given physical device and optional window.
         * It selects appropriate queue families for graphics, compute, and presentation, and creates command pools for each.
         *
         * @param config Configuration struct specifying the physical device and optional window for presentation support.
         * @return std::expected<Device, VHError> An expected containing the created Device on success, or a VHError on failure.
         */
        [[nodiscard]] static std::expected<Device, VHResult> New(const Config& config);
        ~Device();
        Device(const Device& other) = delete;
        Device& operator=(const Device& other) = delete;
        Device(Device&& other) noexcept;
        Device& operator=(Device&& other) noexcept;

        /**
         * @brief Retrieves the underlying Vulkan VkDevice handle.
         *
         * @return VkDevice The Vulkan device handle managed by this object.
         */
        [[nodiscard]] inline VkDevice GetDevice() const { return m_Device; }
        
        /**
         * @brief Gets the graphics queue handle for this device.
         *
         * @return VkQueue handle of the graphics queue.
         */
        [[nodiscard]] inline VkQueue GetGraphicsQueue() const { return m_Queues.GraphicsQueue; }
        /**
         * @brief Gets the compute queue handle for this device.
         *
         * @return VkQueue handle of to the compute queue.
         */
        [[nodiscard]] inline VkQueue GetComputeQueue() const { return m_Queues.ComputeQueue; }
        /**
         * @brief Gets the present queue handle for this device.
         *
         * @return VkQueue handle of to the present queue.
         */
        [[nodiscard]] inline VkQueue GetPresentQueue() const { return m_Queues.PresentQueue; }

        /**
         * @brief Gets the graphics command pool for this device.
         *
         * @return VkCommandPool handle of the graphics command pool.
         */
        [[nodiscard]] inline VkCommandPool GetGraphicsCommandPool() const { return m_CommandPools.GraphicsPool; }
        /**
         * @brief Gets the compute command pool for this device.
         *
         * @return VkCommandPool handle of the compute command pool.
         */
        [[nodiscard]] inline VkCommandPool GetComputeCommandPool() const { return m_CommandPools.ComputePool; }

        /**
         * @brief Gets the physical device associated with this logical device.
         *
         * @return Pointer to the PhysicalDevice object used to create this logical device.
         */
        [[nodiscard]] inline const PhysicalDevice& GetPhysicalDevice() const { return m_PhysicalDevice; }
    private:
        struct QueueFamilyIndices
        {
            uint32_t GraphicsFamily = UINT32_MAX;
            uint32_t ComputeFamily = UINT32_MAX;
            uint32_t PresentFamily = UINT32_MAX;
        };

        struct Queues
        {
            VkQueue GraphicsQueue = NULL;
            VkQueue ComputeQueue = NULL;
            VkQueue PresentQueue = NULL;
        };

        struct CommandPools
        {
            VkCommandPool GraphicsPool = NULL;
            VkCommandPool ComputePool = NULL;
        };

        Device(PhysicalDevice physicalDevice, VkDevice device, Queues queues, CommandPools commandPools)
            : m_PhysicalDevice(physicalDevice), m_Device(device), m_Queues(std::move(queues)), m_CommandPools(std::move(commandPools)) {}

        PhysicalDevice m_PhysicalDevice;
        VkDevice m_Device = nullptr;
        Queues m_Queues;
        CommandPools m_CommandPools;
        
        [[nodiscard]] static QueueFamilyIndices FindQueueFamilies(const PhysicalDevice& physicalDevice, const Window* window);
    };
}