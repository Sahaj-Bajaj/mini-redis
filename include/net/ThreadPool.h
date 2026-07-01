#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace miniredis::net {

// Fixed-size thread pool. Tasks submitted via enqueue() are executed by
// worker threads in FIFO order. Destructor blocks until all queued tasks
// finish — no tasks are silently dropped on shutdown.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t threadCount);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Enqueues a task for execution. Thread-safe.
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;

    void workerLoop();
};

} // namespace miniredis::net