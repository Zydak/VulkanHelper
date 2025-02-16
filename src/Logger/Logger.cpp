#include "Pch.h"
#include "Logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

void VulkanHelper::Logger::Init()
{
	m_Instance = new Logger();

	VH_INFO("Logger initialized!");
}

void VulkanHelper::Logger::Destroy()
{
	delete m_Instance;
	VH_INFO("Logger destroyed!");
}

VulkanHelper::Logger::Logger()
{
	std::remove("VulkanHelper.log");

	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::always);
	consoleSink->set_level(spdlog::level::trace);
	consoleSink->set_pattern("%^[%T] %v%$");

	auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("VulkanHelper.log");
	fileSink->set_level(spdlog::level::trace);
	fileSink->set_pattern("%^[%T] %v%$");

#ifdef DISTRIBUTION
	m_Logger = std::make_shared<spdlog::logger>("Logger", spdlog::sinks_init_list{ fileSink }); // Only use file sink in distribution
#else
	m_Logger = std::make_shared<spdlog::logger>("Logger", spdlog::sinks_init_list{ consoleSink, fileSink });
#endif
}

VulkanHelper::Logger::~Logger()
{

}
