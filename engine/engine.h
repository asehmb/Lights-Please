#pragma once

#include "thread_pool.h"
#include "renderer.h"
#include <memory>
#include "camera.h"

class Engine {
public:
    void run();
    void initialize();
    ~Engine();
    
    // Getters for application access
    Renderer* getRenderer() const { return renderer.get(); }
    
private:
    void process_input();
    void update(float fixed_dt); // Fixed logic (Physics, AI)
    void render(float alpha);    // Variable rendering (Graphics)
    mathplease::Vector2 last_mouse_pos;

    bool is_running = false;
    const float dt = 1.0f / 60.0f; // Target 60Hz for logic
    std::shared_ptr<Camera> camera;
    std::unique_ptr<ThreadPool> thread_pool;
    std::unique_ptr<Renderer> renderer;
};