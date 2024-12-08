#include "pch.h"
#pragma warning(push, 0)
//
// Core assert
//

#ifndef DISTRIBUTION

#if defined(WIN)
#define VK_CORE_ASSERT(condition, ...)\
		if(!(condition)) {\
			VK_CORE_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}
#else
#define VK_CORE_ASSERT(condition, ...)\
		if(!(condition)) {\
			VK_CORE_ERROR(__VA_ARGS__);\
			__builtin_trap();\
		}
#endif

#else
#define VK_CORE_ASSERT(condition, ...)
#endif

#if defined(WIN)
#define VK_CORE_CHECK(condition, ...)\
		if(!(condition)) {\
			VK_CORE_DIST_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}
#else
#define VK_CORE_ASSERT(condition, ...)\
		if(!(condition)) {\
			VK_CORE_DIST_ERROR(__VA_ARGS__);\
			__builtin_trap();\
		}
#endif 

// return value assert
#ifndef DISTRIBUTION

#if defined(WIN)
#define VK_CORE_RETURN_ASSERT(function, value, ...)\
{\
		auto val = function;\
		if(val != value) {\
			VK_CORE_ERROR("Expected: {}, Actual: {}", value, val);\
			VK_CORE_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}\
}
#else
#define VK_CORE_RETURN_ASSERT(function, value, ...)\
{\
		auto val = function;\
		if(val != value) {\
			VK_CORE_ERROR("Expected: {}, Actual: {}", value, val);\
			VK_CORE_ERROR(__VA_ARGS__);\
			__builtin_trap();\
		}\
}
#endif 

#else
#define VK_CORE_RETURN_ASSERT(function, value, ...) function
#endif

//
// Client assert
//

#ifndef DISTRIBUTION

#if defined(WIN)
#define VK_ASSERT(condition, ...)\
		if(!(condition)) {\
			VK_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}
#else
#define VK_ASSERT(condition, ...)\
		if(!(condition)) {\
			VK_ERROR(__VA_ARGS__);\
			__builtin_trap();\
		}
#endif 

#else
#define VK_ASSERT(condition, ...)
#endif

#if defined(WIN)
#define VK_CHECK(condition, ...)\
		if(!(condition)) {\
			VK_DIST_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}
#else
#define VK_CHECK(condition, ...)\
		if(!(condition)) {\
			VK_DIST_ERROR(__VA_ARGS__);\
			__builtin_trap();\
		}
#endif 

// return value assert
#ifndef DISTRIBUTION

#if defined(WIN)
#define VK_RETURN_ASSERT(function, value, ...)\
		if(function != value) {\
			VK_ERROR(__VA_ARGS__);\
			__debugbreak();\
		}
#else
#define VK_RETURN_ASSERT(function, value, ...)\
		if(function != value) {\
			VK_ERROR(__VA_ARGS__);\
			__builtin_trap();\
		}
#endif 

#else
#define VK_RETURN_ASSERT(function, value, ...) function
#endif

#ifndef DISTRIBUTION

#define VK_SET_NAME(objectType, handle, name)\
		Device::SetObjectName(objectType, handle, name);

#else

#define VK_SET_NAME(objectType, handle, name)

#endif

#pragma warning(pop)