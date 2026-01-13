#include "engine.h"
#include "logger.h"
#include "mesh.h"
#include "pipeline.h"
#include "platform.h"
#include <chrono>


void Engine::initialize() {
    is_running = true;
    platform::init();

    // create thread pool with number of hardware threads
    unsigned int thread_count = std::thread::hardware_concurrency();
    thread_pool = std::make_unique<ThreadPool>(thread_count);
    
    // create camera
    camera = std::make_unique<Camera>();


    // create renderer from platform window

    renderer = std::make_unique<Renderer>(platform::get_window_ptr());
    // renderer->createTriangleDrawable();
    
    triangleMaterial = std::make_unique<Material>(
        renderer->getVulkanDevice(),
        renderer->getVmaAllocator(),
        renderer->getDescriptorLayouts()
    );
    trianglePipeline = std::make_shared<GraphicPipeline>(
        renderer->getVulkanDevice(),
        renderer->getRenderPass(),
        "shaders/triangle.vert.spv",
        "shaders/triangle.frag.spv",
        renderer->getSwapChainExtent(),
        &triangleMaterial->pipelineLayout
    );
    triangleMesh = std::make_unique<Mesh>(Mesh::createTriangle(
        renderer->getVulkanDevice(), 
        renderer->getVmaAllocator(), 
        renderer->getCommandPool(), 
        renderer->getGraphicsQueue()
    ));
    triangleMaterial->pipeline = trianglePipeline;

    // When passing it to the renderer, dereference it:
    renderer->createDrawable(triangleMesh.get(), triangleMaterial.get());


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

    camera->update(fixed_dt);
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