#include "engine.h"
#include "logger.h"
#include "platform.h"
#include <chrono>
#include <memory>


void Engine::initialize() {
    is_running = true;
    platform::init(1280, 720);

    // create thread pool with number of hardware threads
    unsigned int thread_count = std::thread::hardware_concurrency();
    thread_pool = std::make_unique<ThreadPool>(thread_count);
    
    // create camera
    camera = std::make_shared<Camera>();

    // create renderer from platform window
    renderer = std::make_unique<Renderer>(platform::get_window_ptr());
    renderer->setCamera(camera);

    LOG_INFO("ENGINE", "Engine initialized with {} threads", thread_count);
}

Engine::~Engine() {
    // Cleanup if necessary
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

        // 'alpha' is how far we are between the current and next physics state
        // This is used for "interpolation" to make motion look smooth
        float alpha = accumulator / dt;
        render(alpha);

        if (platform::should_close()) is_running = false;
    }
    LOG_INFO("ENGINE", "Closing!");
}

void Engine::process_input() {
    platform::poll_events();
    
}

void Engine::update(float fixed_dt) {
    // Update game logic, physics, AI, etc. here
    // sumbit tasks to thread pool if needed

    // camera->update(fixed_dt);
    std::vector<Key> pressed_keys = platform::get_pressed_keys();
    if (!pressed_keys.empty()) {
        float normalizedYaw = camera->yaw;
        for (Key k : pressed_keys) {
            if (k == Key::Escape) {
                is_running = false;
            }
            if (k == Key::W) {
                camera->position.z -= camera->velocity.z * fixed_dt;
            }
            if (k == Key::S) {
                camera->position.z += camera->velocity.z * fixed_dt;
            }
            if (k == Key::A) {
                camera->position.x -= camera->velocity.x * fixed_dt;
            }
            if (k == Key::D) {
                camera->position.x += camera->velocity.x * fixed_dt;
            }
            if (k == Key::Space) {
                camera->position.y += camera->velocity.y * fixed_dt;
            }
            if (k == Key::Shift) {
                camera->position.y -= camera->velocity.y * fixed_dt;
            }
        }
    }
    mathplease::Vector2 relative_mouse_pos = platform::get_relative_mouse_position();
    camera->yaw -= relative_mouse_pos.x * camera->mouseSensitivity * fixed_dt;
    camera->pitch -= relative_mouse_pos.y * camera->mouseSensitivity * fixed_dt;
}

void Engine::render(float alpha) {
    // Render the current frame here
    // Use 'alpha' for interpolating between physics states if needed
    if (renderer) {
        renderer->drawFrame();
    } else{
        LOG_ERR("ENGINE", "Renderer doesnt exist??");
    }
}