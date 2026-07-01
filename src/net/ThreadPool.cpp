#include "net/ThreadPool.h"

#include <utility>

namespace miniredis::net {

ThreadPool::ThreadPool(std::size_t threadCount) {
    workers_.reserve(threadCount);

    for (std::size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    {
        std::scoped_lock lock(mutex_);
        stop_ = true;
    }

    // Wake all threads so they can observe stop_ and exit.
    cv_.notify_all();

    for (auto& thread : workers_) {
        thread.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::scoped_lock lock(mutex_);
        tasks_.push(std::move(task));
    }

    cv_.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock lock(mutex_);

            // Wait until there's a task OR we're shutting down.
            cv_.wait(lock, [this] {
                return !tasks_.empty() || stop_;
            });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // Execute outside the lock so other threads can dequeue concurrently.
        task();
    }
}

} // namespace miniredis::net