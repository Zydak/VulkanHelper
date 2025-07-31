#pragma once

#define COMMAND_BUFFER_CLASS CommandBuffer
#define COMMAND_POOL_CLASS CommandPool
#define DEVICE_CLASS Device
#define FENCE_CLASS Fence
#define IMAGE_CLASS Image
#define INSTANCE_CLASS Instance
#define PHYSICAL_DEVICE_CLASS PhysicalDevice
#define PIPELINE_CLASS Pipeline
#define SEMAPHORE_CLASS Semaphore
#define SWAPCHAIN_CLASS Swapchain
#define VULKAN_MEMORY_ALLOCATOR_CLASS VulkanMemoryAllocator
#define WINDOW_CLASS Window

#define DECLARE_FRIENDS()\
friend class COMMAND_BUFFER_CLASS;\
friend class COMMAND_POOL_CLASS;\
friend class DEVICE_CLASS;\
friend class FENCE_CLASS;\
friend class IMAGE_CLASS;\
friend class INSTANCE_CLASS;\
friend class PHYSICAL_DEVICE_CLASS;\
friend class SEMAPHORE_CLASS;\
friend class SWAPCHAIN_CLASS;\
friend class VULKAN_MEMORY_ALLOCATOR_CLASS;\
friend class WINDOW_CLASS

#define DEFINE_BITWISE_OPERATORS(ENUM_CLASS)\
inline ENUM_CLASS operator|(ENUM_CLASS left, ENUM_CLASS right)\
{\
    return ENUM_CLASS((int)left | int(right));\
}\
\
inline ENUM_CLASS operator&(ENUM_CLASS left, ENUM_CLASS right)\
{\
    return ENUM_CLASS((int)left | int(right));\
}\
\
inline ENUM_CLASS operator~(ENUM_CLASS val)\
{\
    return ENUM_CLASS(~(int)val);\
}\
\
inline ENUM_CLASS operator^(ENUM_CLASS left, ENUM_CLASS right)\
{\
    return ENUM_CLASS((int)left ^ int(right));\
}\
\
inline ENUM_CLASS& operator|=(ENUM_CLASS& left, ENUM_CLASS right)\
{\
    left = left | right;\
    return left;\
}\
\
inline ENUM_CLASS& operator&=(ENUM_CLASS& left, ENUM_CLASS right)\
{\
    left = left & right;\
    return left;\
}\
\
inline ENUM_CLASS& operator^=(ENUM_CLASS& left, ENUM_CLASS right)\
{\
    left = left ^ right;\
    return left;\
}
