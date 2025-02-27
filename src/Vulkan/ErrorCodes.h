#pragma once

namespace VulkanHelper
{
	enum ResultCode {
		Success = 0,
		UnknownError = 1,
		FileNotFound = 2,
		ShaderCompilationFailed = 3,
	};
}