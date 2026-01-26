#include "job_system.h"
#include "logger.h"
#include <thread>

JobSystem::~JobSystem() {
    isRunning = false;
    activeCondition.notify_all();
    for (auto& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void JobSystem::initialize(uint32_t threadCount) {
    if (isRunning) return; // Already initialized

    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
    }

    
    // To safe-guard against 0 (single core machines)
    if (threadCount < 1) threadCount = 1;

    isRunning = true;
    workers.reserve(threadCount);
    
    LOG_INFO("JOB_SYSTEM", "Initializing with %d threads", threadCount);

    for (uint32_t i = 0; i < threadCount; ++i) {
        workers.emplace_back([this, i] { this->workerLoop(i); });
    }
}

void JobSystem::kickJob(const std::function<void()>& job, JobCounter* counter) {
    if (counter) {
        counter->counter++;
    }

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        jobQueue.push_back({job, counter});
    }
    activeCondition.notify_one();
}

void JobSystem::kickJobs(uint32_t count, const std::function<void(uint32_t)>& job, JobCounter* counter) {

    uint32_t jobCount = count;
    
    // Batching heuristic
    
    for (uint32_t i = 0; i < count; ++i) {
        // Capture by value is important here
        kickJob([job, i]() { job(i); }, counter);
    }
}

void JobSystem::waitForCounter(JobCounter* counter) {
    if (!counter) return;

    constexpr int SPIN_ITERS = 1024;
    int spin = 0;

    while (counter->counter.load(std::memory_order_acquire) > 0) {
        if (tryExecuteJob()) {
            spin = 0;
            continue;
        }

        if (spin < SPIN_ITERS) {
            __asm__ volatile("yield");
            ++spin;
        } else {
            std::this_thread::yield(); // fallback if truly starved
        }
    }
}
bool JobSystem::tryExecuteJob() {
    Job job;
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (jobQueue.empty()) {
            return false;
        }
        job = std::move(jobQueue.front());
        jobQueue.pop_front();
    }

    // Execute
    job.task();

    // Decrement counter
    if (job.counter) {
        job.counter->counter--;
    }
    
    return true;
}

void JobSystem::workerLoop(uint32_t threadIndex) {
    while (isRunning) {
        Job job;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            activeCondition.wait(lock, [this] { 
                return !isRunning || !jobQueue.empty(); 
            });

            if (!isRunning && jobQueue.empty()) {
                return;
            }

            if (jobQueue.empty()) {
                continue; // Spurious wake or isRunning false with queue empty
            }

            job = std::move(jobQueue.front());
            jobQueue.pop_front();
        }

        // Execute
        job.task();
        
        // Decrement counter
        if (job.counter) {
            job.counter->counter--;
        }
    }
}
