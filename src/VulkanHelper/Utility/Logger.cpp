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
		std::remove("VulkanHelper.log");

		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);
		consoleSink->set_level(spdlog::level::trace);
		consoleSink->set_pattern("%^[%T] %n: %v%$");
		
		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("VulkanHelper.log");
		fileSink->set_level(spdlog::level::trace);
		fileSink->set_pattern("%^[%T] %n: %v%$");

		s_CoreLogger = std::make_shared<spdlog::logger>("CORE", spdlog::sinks_init_list{ consoleSink, fileSink });
		s_ClientLogger = std::make_shared<spdlog::logger>("APP", spdlog::sinks_init_list{ consoleSink, fileSink });
		s_CoreLogger->set_level(spdlog::level::trace);
		s_ClientLogger->set_level(spdlog::level::trace);
	}

	Logger::~Logger()
	{

	}

}