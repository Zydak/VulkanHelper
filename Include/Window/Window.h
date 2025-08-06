#pragma once

#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Expected.h"

struct GLFWwindow;

namespace VulkanHelper
{
    class PhysicalDevice;
    class Instance;

    /**
     * @class Window
     * @brief RAII wrapper for a GLFWwindow
     */
    class Window
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a Window
         */
        struct Config
        {
            /**
             * @brief Vulkan Instance for creating the window surface
             * @note Must not be nullptr
             */
            VulkanHelper::Instance *Instance = nullptr;

            /**
             * @brief Initial width of the window in pixels
             * @note Must be non-zero
             */
            uint32_t Width = 0;

            /**
             * @brief Initial height of the window in pixels
             * @note Must be non-zero
             */
            uint32_t Height = 0;

            /**
             * @brief Title text for the window's title bar
             */
            const char* Name = "";

            /**
             * @brief Filesystem path to window icon image
             */
            const char* IconPath = "";

            /**
             * @brief Whether the window can be resized by the user
             */
            bool Resizable = true;
        };

        /**
         * @brief Creates a new GLFW window with the specified configuration
         * @param config Configuration parameters for the new window
         * @return Expected containing a Window or error code
         */
        static VulkanHelper::Expected<Window, VHResult> New(const Config& config);

        ~Window();

        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        /**
         * @brief Polls for and processes pending window events
         * @note Must be called regularly to keep window responsive
         */
        static void PollEvents();

        /**
         * @brief Checks whether the user has requested to close the window
         * @return true if close has been requested, false otherwise
         */
        [[nodiscard]] bool WantsToClose() const;

        /**
         * @brief Programmatically signal that the window should close
         */
        void Close();

        /**
         * @brief Checks if the physical device is compatible with this window's surface
         * @param device Vulkan physical device to check for surface compatibility
         * @return true if device supports presentation to this window's surface
         */
        [[nodiscard]] bool IsPhysicalDeviceCompatible(const PhysicalDevice& device) const;

        /**
         * @brief Get the current width of the window
         * @return Width in pixels
         */
        [[nodiscard]] uint32_t GetWidth() const;

        /**
         * @brief Get the current height of the window
         * @return Height in pixels
         */
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * @brief Get the window's title string
         * @return The name/title specified at creation
         */
        [[nodiscard]] const char* GetName() const;

        class Impl;
    private:
        friend class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Window(VulkanHelper::UniquePtr<Impl>&& impl);
    };
} // namespace VulkanHelper