#include "Log/Log.h"
#include "fmt/os.h"

namespace VulkanHelper
{
    Logger& Logger::GetInstance()
    {
        static Logger logger;

        return logger;
    }

    Logger::Logger()
        : m_LogFileOutput(fmt::output_file("VulkanHelper.log", fmt::file::CREATE | fmt::file::WRONLY))
    {
        
    }

    Logger::~Logger()
    {
        m_LogFileOutput.close();
    }
}