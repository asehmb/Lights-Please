#include "engine.h"
#include "platform.h"
#include <chrono>


void Engine::initialize() {
    is_running = true;
    platform::init();
}

void Engine::shutdown() {
    is_running = false;
}
void Engine::run() {
    is_running = true;

    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();
    float accumulator = 0.0f;

    while (is_running) {
        auto current_time = clock::now();
        float frame_time = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        // Prevent "Spiral of Death" (if game lags, don't try to catch up forever)
        if (frame_time > 0.25f) frame_time = 0.25f; 

        accumulator += frame_time;

        process_input();

        // If we have enough time accumulated, run a physics/logic step
        while (accumulator >= dt) {
            update(dt);
            accumulator -= dt;
        }

        // 3. Render
        // 'alpha' is how far we are between the current and next physics state
        // This is used for "interpolation" to make motion look smooth
        float alpha = accumulator / dt;
        render(alpha);

        if (platform::should_close()) is_running = false;
    }
}

void Engine::process_input() {
    platform::poll_events();
}

void Engine::update(float fixed_dt) {
    // Update game logic, physics, AI, etc. here
    // sumbit tasks to thread pool if needed
}

void Engine::render(float alpha) {
    // Render the current frame here
    // Use 'alpha' for interpolating between physics states if needed
}