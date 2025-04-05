#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace VulkanHelper
{
	class Device;

	class ThreadPool
	{
	public:
		struct CreateInfo
		{
			Device* Device = nullptr;
			uint32_t threadCount;
		};

		ThreadPool() = default;
		ThreadPool(const CreateInfo& createInfo);
		~ThreadPool();

		void Init(const CreateInfo& createInfo);

		explicit ThreadPool(const ThreadPool& other) = delete;
		explicit ThreadPool(ThreadPool&& other) noexcept;
		ThreadPool& operator=(const ThreadPool& other) = delete;
		ThreadPool& operator=(ThreadPool&& other) noexcept;

		template<typename T, typename ...Args>
		void PushTask(T&& task, Args&& ... args)
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Tasks.emplace(std::bind(std::forward<T>(task), std::forward<Args>(args)...));
			lock.unlock();
			m_CV.notify_one();
		}

		inline uint32_t GetThreadCount() const { return (uint32_t)m_WorkerThreads.size(); }

		inline size_t TasksLeft()
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			return m_Tasks.size();
		};

	private:
		Device* m_Device = nullptr;

		std::vector<std::thread> m_WorkerThreads;
		std::queue<std::function<void()>> m_Tasks;

		std::mutex m_Mutex;
		std::condition_variable m_CV;
		bool m_Stop = false;

		void Move(ThreadPool&& other);
		void Destroy();
	};
}