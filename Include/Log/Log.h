#pragma once
#include "fmt/base.h"
#include <fmt/os.h>
#include <fmt/format.h>
#include <fmt/color.h>

#include <string>

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

        static Logger& GetInstance();

        template<typename ...T>
        void Log(Verbosity verbosity, const fmt::text_style& style, const char* function, int line, const char* str, T&& ...args)
        {
            // First add function name and line number
            std::string formattedStr = fmt::format(fmt::runtime("{} ({}):\n\t {}\n"), function, line, str);
            // Then add user specified arguments, this has to be done separately
            formattedStr = fmt::format(fmt::runtime(formattedStr), args...);
            
            if (m_ConsoleVerbosity >= verbosity)
                fmt::print(style, fmt::runtime(formattedStr));

            if (m_FileVerbosity >= verbosity)
                m_LogFileOutput.print(fmt::runtime(formattedStr));
        }

        inline void SetVerbosityConsole(Verbosity verbosity) { m_ConsoleVerbosity = verbosity; }
        inline void SetVerbosityFile(Verbosity verbosity) { m_FileVerbosity = verbosity; }
    
    private:
        Logger();
        ~Logger();

        Logger(const Logger& other) = delete;
        Logger& operator=(const Logger& other) = delete;
        Logger(Logger&& other) = delete;
        Logger& operator=(Logger&& other) = delete;

        fmt::ostream m_LogFileOutput;
        Verbosity m_FileVerbosity = Verbosity::VTrace;
        Verbosity m_ConsoleVerbosity = Verbosity::VTrace;
    };
}

#define VH_LOG_FATAL(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::white) | fmt::bg(fmt::color::crimson) | fmt::emphasis::bold, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_ERROR(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::red) | fmt::emphasis::bold, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_WARN(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::yellow), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_INFO(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::green), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_DEBUG(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::blue), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define VH_LOG_TRACE(...) ::VulkanHelper::Logger::GetInstance().Log(::VulkanHelper::Logger::VFatal, fmt::fg(fmt::color::white), __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)

#define VH_ASSERT(condition, ...)\
if (!(condition))\
{\
    VH_LOG_FATAL(__VA_ARGS__);\
    exit(1);\
}