#pragma once

#define COMMAND_BUFFER_CLASS CommandBuffer
#define COMMAND_POOL_CLASS CommandPool
#define DEVICE_CLASS Device
#define FENCE_CLASS Fence
#define INSTANCE_CLASS Instance
#define PHYSICAL_DEVICE_CLASS PhysicalDevice
#define SEMAPHORE_CLASS Semaphore
#define SWAPCHAIN_CLASS Swapchain
#define WINDOW_CLASS Window

#define DECLARE_FRIENDS()\
friend class COMMAND_BUFFER_CLASS;\
friend class COMMAND_POOL_CLASS;\
friend class DEVICE_CLASS;\
friend class FENCE_CLASS;\
friend class INSTANCE_CLASS;\
friend class PHYSICAL_DEVICE_CLASS;\
friend class SEMAPHORE_CLASS;\
friend class SWAPCHAIN_CLASS;\
friend class WINDOW_CLASS
