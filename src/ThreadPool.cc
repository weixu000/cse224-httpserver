#include "spdlog/spdlog.h"
#include "ThreadPool.hpp"

ThreadPool::ThreadPool(unsigned int num) {
    num = num > 0 ? num : std::thread::hardware_concurrency();
    for (unsigned int i = 0; i != num; ++i) {
        threads.emplace_back([this] { workLoop(); });
    }
    spdlog::info("Spawned threads: {}", num);
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(mutex);
        shouldStop = true;
    }
    cv.notify_all();
    for (auto &t:threads) {
        t.join();
    }
}

void ThreadPool::addTask(ThreadPool::Task f) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (shouldStop) {
            throw std::runtime_error("Cannot add task after stopped");
        }
        queue.push(std::move(f));
    }
    cv.notify_one();
}

void ThreadPool::workLoop() {
    while (true) {
        Task f;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this] { return !queue.empty() || shouldStop; });
            if (shouldStop && queue.empty()) { // if stopped, handle remaining tasks
                break;
            }
            f = std::move(queue.front());
            queue.pop();
        }
        f();
    }
}
