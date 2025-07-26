#pragma once

#include <stdint.h>
#include <string>
#include <expected>

#include "Core/Error.h"

struct GLFWwindow;

namespace VulkanHelper
{
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
            std::string Name = "";

            /**
             * @brief Filesystem path to an image to use as the window icon.
             * @note If empty or if loading fails, the default GLFW icon is used.
             */
            std::string IconPath = "";

            /**
             * @brief Whether the window can be resized by the user.
             * @note If true, window decorations will allow manual resizing.
             */
            bool Resizable = true;
        };

        /**
         * @brief Initialize GLFW (if not already initialized) and create a new GLFW window with the specified configuration.
         *
         * @param config Configuration parameters for the new window.
         * @return On success, returns std::expected containing a Window.
         *         On failure, returns std::expected containing a VHError::Fail
         */
        static std::expected<Window, VHError> New(const Config& config);

        ~Window();

        // Deleted copy constructor and copy-assignment to enforce unique ownership.
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
        [[nodiscard]] bool WantsToClose();

        /**
         * @brief Programmatically signal that the window should close.
         *
         * Equivalent to the user clicking the close button; subsequent
         * calls to WantsToClose() will return true.
         */
        void Close();

        //------------------------------------------------------------------------
        // Inline Getters
        //------------------------------------------------------------------------

        /**
         * @brief Retrieve the current width of the window.
         *
         * @return Width in pixels.
         */
        [[nodiscard]] inline uint32_t GetWidth() const { return m_Width; }

        /**
         * @brief Retrieve the current height of the window.
         *
         * @return Height in pixels.
         */
        [[nodiscard]] inline uint32_t GetHeight() const { return m_Height; }

        /**
         * @brief Retrieve the window's title string.
         *
         * @return The name/title originally specified at creation.
         */
        [[nodiscard]] inline std::string GetName() const { return m_Name; }

    private:
        // Constructable only by calling New(...)
        Window(GLFWwindow* window, std::string&& name, uint32_t width, uint32_t height)
            : m_Window(window),
            m_Name(std::move(name)),
            m_Width(width),
            m_Height(height)
        {}

        GLFWwindow*   m_Window;  ///< Underlying GLFW window handle.
        std::string   m_Name;    ///< User-specified window title.
        uint32_t      m_Width;   ///< Current window width in pixels.
        uint32_t      m_Height;  ///< Current window height in pixels.
    };
} // namespace VulkanHelper