#pragma once

#include <fmt/base.h>
#include <fmt/os.h>
#include <fmt/format.h>
#include <fmt/color.h>

namespace VulkanHelper
{
    class Logger
    {
    public:
        enum Verbosity
        {
            VNone = -1,
            VFatal = 0,
            VError = 1,
            VWarn = 2,
            VInfo = 3,
            VDebug = 4,
            VTrace = 5
        };

        /**
         * @brief Gets the singleton instance of the Logger.
         *
         * This function returns a reference to the global Logger instance.
         *
         * @return Logger& Reference to the singleton Logger instance.
         */
        static Logger& GetInstance()
        {
            static Logger logger;

            return logger;
        }

        /**
         * @brief Logs a formatted message with the specified verbosity and style.
         *
         * This templated function logs a message to the console and/or file, depending on the set verbosity levels.
         * The message is formatted using fmtlib, and includes the function name and line number for context.
         *
         * @tparam T Variadic template arguments for text.
         * @param verbosity The verbosity level of the message (e.g., error, warning, info).
         * @param style The fmt::text_style to use for console output.
         * @param function The name of the calling function (usually __PRETTY_FUNCTION__).
         * @param line The line number in the source file.
         * @param str The format string for the log message.
         * @param args Arguments to be formatted into the log message.
         */
        template<typename ...T>
        void Log(Verbosity verbosity, const fmt::text_style& style, const char* function, int line, const char* str, T&& ...args)
        {
            // First add function name and line number
            std::string formattedStr = fmt::format(fmt::runtime("{} ({}):\n\t {}\n"), function, line, str);
            // Then add user specified arguments, this has to be done separately
            formattedStr = fmt::format(fmt::runtime(formattedStr), args...);
            
            if (m_ConsoleVerbosity >= verbosity)
                fmt::print(style, "{}", formattedStr);

            if (m_FileVerbosity >= verbosity)
                m_LogFileOutput.print("{}", formattedStr);
        }

        /**
         * @brief Sets the minimum verbosity level for console output.
         *
         * Messages below this verbosity will not be printed to the console.
         *
         * @param verbosity The minimum verbosity level for console output.
         */
        inline void SetVerbosityConsole(Verbosity verbosity) { m_ConsoleVerbosity = verbosity; }

        /**
         * @brief Sets the minimum verbosity level for file output.
         *
         * Messages below this verbosity will not be written to the log file.
         *
         * @param verbosity The minimum verbosity level for file output.
         */
        inline void SetVerbosityFile(Verbosity verbosity) { m_FileVerbosity = verbosity; }
    
    private:
        Logger()
            : m_LogFileOutput(fmt::output_file("VulkanHelper.log", fmt::file::CREATE | fmt::file::WRONLY))
        {
            
        }
        ~Logger()
        {
            m_LogFileOutput.close();
        }

        Logger(const Logger& other) = delete;
        Logger& operator=(const Logger& other) = delete;
        Logger(Logger&& other) = delete;
        Logger& operator=(Logger&& other) = delete;

        fmt::ostream m_LogFileOutput;
        Verbosity m_FileVerbosity = Verbosity::VTrace;
        Verbosity m_ConsoleVerbosity = Verbosity::VTrace;
    };
}

#ifdef WIN32
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define VH_LOG_FATAL(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::white) | fmt::bg(fmt::color::crimson) | fmt::emphasis::bold, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_ERROR(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VError, fmt::fg(fmt::color::red) | fmt::emphasis::bold, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_WARN(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VWarn, fmt::fg(fmt::color::yellow), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_INFO(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VInfo, fmt::fg(fmt::color::green), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_DEBUG(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VDebug, fmt::fg(fmt::color::blue), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_TRACE(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VTrace, fmt::fg(fmt::color::white), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)

#define VH_ASSERT(condition, ...)\
if (!(condition))\
{\
    VH_LOG_FATAL(__VA_ARGS__);\
    std::terminate();\
}
