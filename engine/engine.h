#pragma once

#include "job_system.h"
#include "renderer/renderer.h"
#include <memory>
#include "camera.h"
#include "entity/entity.h"
#include "entity/systems.h"
#include "entity/renderSystem.h"

class Engine {
public:
    void run();
    void initialize();
    ~Engine();
    
    // Getters for application access
    Renderer* getRenderer() const { return renderer.get(); }
    EntityManager* getEntityManager() { return entity_manager_ptr.get(); }
    RenderSystem* getRenderSystem() { return &renderSystem; }
    
private:
    void process_input();
    void update(float fixed_dt); // Fixed logic (Physics, AI)
    void render(float alpha);    // Variable rendering (Graphics)
    mathplease::Vector2 last_mouse_pos;
    std::unique_ptr<EntityManager> entity_manager_ptr;
    GravitySystem gravitySystem;
    RenderSystem renderSystem;


    bool is_running = false;
    const float dt = 1.0f / 60.0f; // Target 60Hz for logic
    std::shared_ptr<Camera> camera;
    std::unique_ptr<JobSystem> job_system;
    std::unique_ptr<Renderer> renderer;
};
