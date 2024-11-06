#include "pch.h"

#include "Logger.h"
#include "Utility/Utility.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace VulkanHelper
{
	std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

	void Logger::Init()
	{
		//std::remove("VulkanHelper.log");
		//std::vector<spdlog::sink_ptr> sinks;
		//
		//sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
		//sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("VulkanHelper.log"));

		spdlog::set_pattern("%^[%T] %n: %v%$");
		//s_CoreLogger = std::make_shared<spdlog::logger>("VULTURE_CORE", begin(sinks), end(sinks));
		s_CoreLogger = spdlog::stdout_color_mt("VulkanHelper CORE");
		s_CoreLogger->set_level(spdlog::level::trace);

		//s_ClientLogger = std::make_shared<spdlog::logger>("APP", begin(sinks), end(sinks));
		s_ClientLogger = spdlog::stdout_color_mt("APP");
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	Logger::~Logger()
	{

	}

}