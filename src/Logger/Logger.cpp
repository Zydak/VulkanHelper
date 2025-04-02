#include "Pch.h"
#include "Logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

void VulkanHelper::Logger::Init()
{
	if (s_Instance != nullptr)
		Destroy();

	s_Instance = new Logger();

	VH_INFO("Logger initialized!");
}

void VulkanHelper::Logger::Destroy()
{
	VH_ASSERT(s_Instance != nullptr, "Logger is not initialized!");

	VH_INFO("Logger destroyed!");
	delete s_Instance;
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
	m_Logger = std::make_shared<spdlog::logger>("Logger", spdlog::sinks_init_list{ fileSink }); // Use only file sink in distribution since writing to console is quite slow
#else
	m_Logger = std::make_shared<spdlog::logger>("Logger", spdlog::sinks_init_list{ consoleSink, fileSink });
#endif
}

VulkanHelper::Logger::~Logger()
{

}
