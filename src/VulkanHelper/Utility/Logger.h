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
#define VK_CORE_ERROR(...)
#define VK_CORE_WARN(...)
#define VK_CORE_INFO(...)
#define VK_CORE_TRACE(...)
#else
#define VK_CORE_ERROR(...)	::VulkanHelper::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define VK_CORE_WARN(...)	::VulkanHelper::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define VK_CORE_INFO(...)	::VulkanHelper::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define VK_CORE_TRACE(...)	::VulkanHelper::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#endif
#define VK_CORE_DIST_ERROR(...)	::VulkanHelper::Logger::GetCoreLogger()->error(__VA_ARGS__)

// Client log macros
#ifdef DISTRIBUTION
#define VK_ERROR(...)
#define VK_WARN(...)
#define VK_INFO(...)
#define VK_TRACE(...)
#else
#define VK_ERROR(...)	::VulkanHelper::Logger::GetClientLogger()->error(__VA_ARGS__)
#define VK_WARN(...)	::VulkanHelper::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define VK_INFO(...)	::VulkanHelper::Logger::GetClientLogger()->info(__VA_ARGS__)
#define VK_TRACE(...)	::VulkanHelper::Logger::GetClientLogger()->trace(__VA_ARGS__)
#endif
#define VK_DIST_ERROR(...)	::VulkanHelper::Logger::GetClientLogger()->error(__VA_ARGS__)