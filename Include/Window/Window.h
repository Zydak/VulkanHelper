#pragma once

#include "Core/Error.h"
#include "Core/UniquePtr.h"
#include "Core/Expected.h"
#include "Core/Macros.h"

struct GLFWwindow;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

namespace VulkanHelper
{
    class PhysicalDevice;
    class Instance;

    /**
     * @class Window
     * @brief RAII wrapper for a GLFWwindow.
     *
     * Manages the lifetime of a GLFWwindow, providing creation, event polling,
     * querying, and destruction functionality.
     */
    class Window
    {
    public:
        /**
         * @struct Config
         * @brief Configuration parameters for creating a Window instance.
         *
         * Fill in the desired width, height, window title, optional icon path,
         * and whether the window should be resizable. Pass this struct to
         * Window::New() to create a window.
         */
        struct Config
        {
            /**
             * @brief Pointer to the Vulkan Instance this window will be associated with.
             * @note Must not be null; the window will use this instance to create a VkSurfaceKHR.
             */
            VulkanHelper::Instance *Instance = nullptr;

            /**
             * @brief The initial width of the window in pixels.
             * @note Must be non-zero for window creation to succeed.
             */
            uint32_t Width = 0;

            /**
             * @brief The initial height of the window in pixels.
             * @note Must be non-zero for window creation to succeed.
             */
            uint32_t Height = 0;

            /**
             * @brief The title text to display in the window's title bar.
             * @note If empty, the window will have no title.
             */
            const char* Name = "";

            /**
             * @brief Filesystem path to an image to use as the window icon.
             * @note If empty or if loading fails, the default GLFW icon is used.
             */
            const char* IconPath = "";

            /**
             * @brief Whether the window can be resized by the user.
             * @note If true, window decorations will allow manual resizing.
             */
            bool Resizable = true;
        };

        /**
         * @brief Initialize GLFW (if not already initialized) and create a new GLFW window with the specified configuration.

         * This static factory function attempts to create a wrapper around GLFWwindow according to the provided configuration.
         * If successful, it returns a Window object; otherwise, it returns a VHError.
         *
         * @param config Configuration parameters for the new window.
         * @return On success, returns VulkanHelper::Expected containing a Window.
         *         On failure, returns stdVulkanHelper::Expected containing a VHError::Fail
         */
        static VulkanHelper::Expected<Window, VHResult> New(const Config& config);

        ~Window();

        Window(const Window& other) = delete;
        Window& operator=(const Window& other) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        /**
         * @brief Polls for and processes pending window events.
         *
         * Must be called regularly (once per frame) to ensure the window
         * remains responsive to input and OS events.
         */
        static void PollEvents();

        /**
         * @brief Checks whether the user has requested to close the window.
         *
         * This typically becomes true when the user clicks the close button
         * or issues an equivalent OS-level command.
         *
         * @return true if a close has been requested, false otherwise.
         */
        [[nodiscard]] bool WantsToClose() const;

        /**
         * @brief Programmatically signal that the window should close.
         *
         * Equivalent to the user clicking the close button; subsequent
         * calls to WantsToClose() will return true.
         */
        void Close();

        /**
         * @brief Checks if the given physical device is compatible with this window's surface.
         *
         * This function queries the provided Vulkan physical device to determine if it supports presentation
         * to the window's surface (e.g., via vkGetPhysicalDeviceSurfaceSupportKHR). This is typically used
         * to filter out devices that cannot present images to the window, ensuring that only compatible GPUs
         * are considered for rendering.
         *
         * @param device The Vulkan physical device to check for surface compatibility.
         * @return true if the device supports presentation to this window's surface; false otherwise.
         */
        [[nodiscard]] bool IsPhysicalDeviceCompatible(const PhysicalDevice& device) const;

        //------------------------------------------------------------------------
        // Inline Getters
        //------------------------------------------------------------------------

        /**
         * @brief Retrieve the current width of the window.
         *
         * @return Width in pixels.
         */
        [[nodiscard]] uint32_t GetWidth() const;

        /**
         * @brief Retrieve the current height of the window.
         *
         * @return Height in pixels.
         */
        [[nodiscard]] uint32_t GetHeight() const;

        /**
         * @brief Retrieve the window's title string.
         *
         * @return The name/title originally specified at creation.
         */
        [[nodiscard]] const char* GetName() const;

        /**
         * @brief Retrieve the window's surface.
         *
         * @return VkSurfaceKHR of the window.
         */
        [[nodiscard]] VkSurfaceKHR GetSurface() const;

        private:

        class Impl;
        VulkanHelper::UniquePtr<Impl> m_Impl;

        Window(VulkanHelper::UniquePtr<Impl>&& impl);

        #undef WINDOW_CLASS
        DECLARE_FRIENDS();
        #define WINDOW_CLASS Window
    };
} // namespace VulkanHelper