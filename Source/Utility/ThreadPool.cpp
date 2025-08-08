#include "Utility/ThreadPool.h"
#include "Log/Log.h"

namespace VulkanHelper
{
    ThreadPool::ThreadPool(size_t numThreads)
        : m_Stop(false)
    {
        // If numThreads is 0, use hardware concurrency
        if (numThreads == 0)
        {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0)
            {
                VH_LOG_WARN("Hardware concurrency is 0, using 1 thread instead, things may be slow!");
                numThreads = 1; // Fallback to 1 thread if hardware_concurrency fails
            }
        }

        m_Workers.Reserve(numThreads);
        for (size_t i = 0; i < numThreads; ++i)
        {
            m_Workers.EmplaceBack(&ThreadPool::WorkerThread, this);
        }

        VH_LOG_INFO("ThreadPool initialized with {} worker threads", numThreads);
    }

    ThreadPool::~ThreadPool()
    {
        Stop();
    }

    ThreadPool::ThreadPool(ThreadPool&& other) noexcept
        : m_Workers(VulkanHelper::Move(other.m_Workers))
        , m_Tasks(VulkanHelper::Move(other.m_Tasks))
        , m_QueueMutex()
        , m_Condition()
        , m_Stop(other.m_Stop.load())
    {
        other.m_Stop = true;
    }

    ThreadPool& ThreadPool::operator=(ThreadPool&& other) noexcept
    {
        if (this != &other)
        {
            Stop();

            // Move data from other
            m_Workers = VulkanHelper::Move(other.m_Workers);
            m_Tasks = VulkanHelper::Move(other.m_Tasks);
            m_Stop = other.m_Stop.load();

            // Mark other as stopped
            other.m_Stop = true;
        }
        return *this;
    }

    void ThreadPool::WorkerThread()
    {
        while (true)
        {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);

                // Wait for a task or stop signal
                m_Condition.wait(lock, [this]() { return m_Stop || !m_Tasks.empty(); });

                // Exit if stopped and no more tasks
                if (m_Stop && m_Tasks.empty())
                {
                    return;
                }

                // Get the next task
                task = VulkanHelper::Move(m_Tasks.front());
                m_Tasks.pop();
            }

            // Execute the task outside the lock
            task();
        }
    }

    void ThreadPool::Stop()
    {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_Stop = true;
        }

        // Wake up all threads
        m_Condition.notify_all();

        // Wait for all threads to finish
        for (size_t i = 0; i < m_Workers.Size(); ++i)
        {
            if (m_Workers[i].joinable())
            {
                m_Workers[i].join();
            }
        }

        m_Workers.Clear();
    }
} // namespace VulkanHelper