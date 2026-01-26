#pragma once

#include <functional>
#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>
#include <deque>
#include <mutex>

struct JobCounter {
    std::atomic<int> counter{0};
};

class JobSystem {
public:
    JobSystem() = default;
    ~JobSystem();

    void initialize(uint32_t threadCount = 0);
    
    // Kick a job. 
    // If counter is provided, it must be initialized (usually to 0, or result of previous adds).
    // The system increments the counter before queuing and decrements upon completion.
    void kickJob(const std::function<void()>& job, JobCounter* counter = nullptr);
    
    // Kick a set of jobs (Parallel For)
    // Divides 'count' items among threads.
    void kickJobs(uint32_t count, const std::function<void(uint32_t)>& job, JobCounter* counter = nullptr);

    // Wait for a counter to reach zero.
    // While waiting, the calling thread will help execute jobs to prevent deadlocks.
    void waitForCounter(JobCounter* counter);

private:
    void workerLoop(uint32_t threadIndex);

    // Execute one job from queue. Returns true if job executed.
    bool tryExecuteJob(); 


    struct Job {
        std::function<void()> task;
        JobCounter* counter = nullptr;
    };

    std::vector<std::thread> workers;
    std::deque<Job> jobQueue;
    std::mutex queueMutex;
    std::condition_variable activeCondition;
    
    std::atomic<bool> isRunning{false};
};
