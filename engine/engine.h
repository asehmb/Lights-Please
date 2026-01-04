#pragma once
#include "thread_pool.h"

class Engine {
public:
    void run();
    void initialize();
    void shutdown();
private:
    void process_input();
    void update(float fixed_dt); // Fixed logic (Physics, AI)
    void render(float alpha);    // Variable rendering (Graphics)

    bool is_running = false;
    const float dt = 1.0f / 60.0f; // Target 60Hz for logic
    std::unique_ptr<ThreadPool> thread_pool;
};