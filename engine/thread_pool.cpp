#include "thread_pool.h"

ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    // create worker threads
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    // lock the queue mutex
                    std::unique_lock<std::mutex> lock(this->queue_mutex);

                    // wait for a task or stop signal
                    condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    // exit if stopping and no tasks left
                    if (this->stop && this->tasks.empty()) return;

                    // get the next task
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                // execute the task
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        // lock the queue mutex and set stop flag
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(const std::function<void()>& task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(task);
    }
    condition.notify_one();
}
