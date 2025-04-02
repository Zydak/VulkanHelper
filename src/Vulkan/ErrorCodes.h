#pragma once

namespace VulkanHelper
{
	enum ResultCode {
		Success = 0,
		NotReady = 1,
		Timeout = 2,
		EventSet = 3,
		EventReset = 4,
		Incomplete = 5,
		OutOfHostMemory = -1,
		OutOfDeviceMemory = -2,
		InitializationFailed = -3,
		DeviceLost = -4,
		MemoryMapFailed = -5,
		LayerNotPresent = -6,
		ExtensionNotPresent = -7,
		FeatureNotPresent = -8,
		IncompatibleDriver = -9,
		TooManyObjects = -10,
		FormatNotSupported = -11,
		FragmentedPool = -12,
		UnknownError = -13,

		ShaderCompilationFailed = -14,
		FileNotFound = -15,
	};
}