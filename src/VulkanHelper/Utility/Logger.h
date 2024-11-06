#pragma once

#include "pch.h"

#include "spdlog/spdlog.h"

namespace VulkanHelper
{
	class Logger
	{
	public:
		static void Init();
		~Logger();

		inline static std::shared_ptr<spdlog::logger> GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger> GetClientLogger() { return s_CoreLogger; }
	private:
		Logger() {};

		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log macros
#ifdef DISTRIBUTION
#define VL_CORE_ERROR(...)
#define VL_CORE_WARN(...)
#define VL_CORE_INFO(...)
#define VL_CORE_TRACE(...)
#else
#define VL_CORE_ERROR(...)	::VulkanHelper::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define VL_CORE_WARN(...)	::VulkanHelper::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define VL_CORE_INFO(...)	::VulkanHelper::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define VL_CORE_TRACE(...)	::VulkanHelper::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#endif
#define VL_CORE_DIST_ERROR(...)	::VulkanHelper::Logger::GetCoreLogger()->error(__VA_ARGS__)

// Client log macros
#ifdef DISTRIBUTION
#define VL_ERROR(...)
#define VL_WARN(...)
#define VL_INFO(...)
#define VL_TRACE(...)
#else
#define VL_ERROR(...)	::VulkanHelper::Logger::GetClientLogger()->error(__VA_ARGS__)
#define VL_WARN(...)	::VulkanHelper::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define VL_INFO(...)	::VulkanHelper::Logger::GetClientLogger()->info(__VA_ARGS__)
#define VL_TRACE(...)	::VulkanHelper::Logger::GetClientLogger()->trace(__VA_ARGS__)
#endif
#define VL_DIST_ERROR(...)	::VulkanHelper::Logger::GetClientLogger()->error(__VA_ARGS__)