#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <vector>

class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(unsigned int num = 0);

    virtual ~ThreadPool();

    void addTask(Task f);

private:
    std::queue<Task> queue;
    std::mutex mutex;
    std::condition_variable cv;

    bool shouldStop = false;
    std::vector<std::thread> threads;

    void workLoop();
};


#endif //THREADPOOL_HPP
