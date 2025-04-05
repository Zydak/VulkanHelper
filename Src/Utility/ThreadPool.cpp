#include "pch.h"
#include "ThreadPool.h"
#include "Vulkan/Device.h"

namespace VulkanHelper
{

	ThreadPool::ThreadPool(const CreateInfo& createInfo)
	{
		Init(createInfo);
	}

	ThreadPool::ThreadPool(ThreadPool&& other) noexcept
	{
		if (this == &other)
			return;

		Destroy();

		Move(std::move(other));
	}

	VulkanHelper::ThreadPool& ThreadPool::operator=(ThreadPool&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		Move(std::move(other));

		return *this;
	}

	ThreadPool::~ThreadPool()
	{
		Destroy();
	}

	void ThreadPool::Init(const CreateInfo& createInfo)
	{
		Destroy();

		m_Device = createInfo.Device;

		for (uint32_t i = 0; i < createInfo.threadCount; i++)
		{
			m_WorkerThreads.emplace_back([this] {
				m_Device->CreateCommandPoolsForThread();
				while (true)
				{
					std::unique_lock<std::mutex> lock(m_Mutex);
					m_CV.wait(lock, [this] { return m_Stop || !m_Tasks.empty(); });
					if (m_Stop && m_Tasks.empty())
						return;

					auto task = std::move(m_Tasks.front());
					m_Tasks.pop();
					lock.unlock();
					task();
				}
				});
		}
	}

	void ThreadPool::Move(ThreadPool&& other)
	{
		m_WorkerThreads = std::move(other.m_WorkerThreads);
		m_Tasks = std::move(other.m_Tasks);
		m_Stop = other.m_Stop;
		m_Device = other.m_Device;
	}

	void ThreadPool::Destroy()
	{
		if (m_Device == nullptr)
			return;

		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Stop = true;
		lock.unlock();
		m_CV.notify_all();
		for (auto& worker : m_WorkerThreads)
		{
			worker.join();
		}
	}

}