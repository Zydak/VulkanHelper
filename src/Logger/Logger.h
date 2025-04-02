#pragma once
#include "spdlog/spdlog.h"

namespace VulkanHelper
{

	class Logger
	{
	public:

		static void Init();
		static void Destroy();

		inline static Logger* GetInstance() { return s_Instance; };
		inline std::shared_ptr<spdlog::logger>& GetLogger() { return m_Logger; };

		inline void SetLevel(spdlog::level::level_enum level) { m_Logger->set_level(level); }

	private:
		Logger();
		~Logger();

		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(Logger&&) = delete;

		std::shared_ptr<spdlog::logger> m_Logger;

		inline static Logger* s_Instance;
	};

} // namespace VulkanHelper

#define VH_ERROR(...)	::VulkanHelper::Logger::GetInstance()->GetLogger()->error(__VA_ARGS__)
#define VH_WARN(...)	::VulkanHelper::Logger::GetInstance()->GetLogger()->warn(__VA_ARGS__)
#define VH_INFO(...)	::VulkanHelper::Logger::GetInstance()->GetLogger()->info(__VA_ARGS__)
#define VH_TRACE(...)	::VulkanHelper::Logger::GetInstance()->GetLogger()->trace(__VA_ARGS__)

#ifndef DISTRIBUTION
#define VH_ASSERT(condition, ...)\
		if(!(condition)) {\
			VH_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}
#else
#define VH_ASSERT(condition, ...)
#endif

#define VH_CHECK(condition, ...)\
		if(!(condition)) {\
			VH_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}