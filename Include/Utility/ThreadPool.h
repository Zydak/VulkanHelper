#pragma once

#include "Core/Vector.h"
#include "Core/UniquePtr.h"
#include "Core/Move.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <queue>
#include <atomic>

namespace VulkanHelper
{
    /**
     * @brief A thread pool implementation for executing tasks asynchronously.
     * 
     * The ThreadPool manages a configurable number of worker threads that execute
     * submitted tasks. Worker threads are put to sleep when there are no tasks
     * and are awakened when new tasks are submitted.
     */
    class ThreadPool
    {
    public:
        /**
         * @brief Constructs a ThreadPool with the specified number of worker threads.
         * 
         * @param numThreads Number of worker threads to create. If 0, uses hardware_concurrency.
         */
        explicit ThreadPool(size_t numThreads = 0);

        /**
         * @brief Destructor. Stops all worker threads and waits for them to finish.
         */
        ~ThreadPool();

        /**
         * @brief Delete copy constructor.
         */
        ThreadPool(const ThreadPool& other) = delete;

        /**
         * @brief Delete copy assignment operator.
         */
        ThreadPool& operator=(const ThreadPool& other) = delete;

        /**
         * @brief Move constructor.
         * 
         * @param other ThreadPool to move from.
         */
        ThreadPool(ThreadPool&& other) noexcept;

        /**
         * @brief Move assignment operator.
         * 
         * @param other ThreadPool to move from.
         * @return Reference to this ThreadPool.
         */
        ThreadPool& operator=(ThreadPool&& other) noexcept;

        /**
         * @brief Submits a function to be executed asynchronously on a worker thread.
         * 
         * @tparam F Function type.
         * @tparam Args Argument types for the function.
         * @param function Function to execute.
         * @param args Arguments to pass to the function.
         * @return std::future containing the result of the function execution.
         */
        template<typename F, typename... Args>
        auto Submit(F&& function, Args&&... args) -> std::future<decltype(function(args...))>
        {
            using ReturnType = decltype(function(args...));

            // Create a packaged task that wraps the function and its arguments
            auto task = std::make_shared<std::packaged_task<ReturnType()>>(
                std::bind(std::forward<F>(function), std::forward<Args>(args)...)
            );

            std::future<ReturnType> result = task->get_future();

            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                
                // Don't allow adding tasks after stopping
                VH_ASSERT(!m_Stop.load(), "Cannot submit tasks to a stopped ThreadPool");

                // Add the task to the queue
                m_Tasks.emplace([task]() { (*task)(); });
            }

            // Notify one waiting worker thread
            m_Condition.notify_one();
            return result;
        }

        /**
         * @brief Gets the number of worker threads in the pool.
         * 
         * @return Number of worker threads.
         */
        size_t GetThreadCount() const { return m_Workers.Size(); }

        /**
         * @brief Checks if the thread pool has been stopped.
         * 
         * @return True if the thread pool is stopped, false otherwise.
         */
        bool IsStopped() const { return m_Stop; }

    private:
        /**
         * @brief Main worker thread function.
         * 
         * Continuously processes tasks from the queue until stopped.
         */
        void WorkerThread();

        /**
         * @brief Stops all worker threads and cleans up resources.
         */
        void Stop();

        // Worker threads
        VulkanHelper::Vector<std::thread> m_Workers;

        // Task queue
        std::queue<std::function<void()>> m_Tasks;

        // Synchronization
        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        std::atomic<bool> m_Stop;
    };
} // namespace VulkanHelper