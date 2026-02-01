
#include <SDL.h>
// #include <vector>
#include "engine/engine.h"
// #include "engine/geometry.h"
#include "engine/logger.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/material.hpp"
#include "engine/renderer/pipeline.h"
#include "engine/renderer/texture.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "engine/loadModel.h"
#include <memory>


int main() {
    // Initialize and run the engine
    Engine engine;
    engine.initialize();
    
    // Load application-specific model
    Mesh::MeshData meshData;
    modelsPlease::loadModelFromOBJ("models/Minion.obj", meshData.vertices, meshData.indices);
    
    // Create application-specific material
    std::unique_ptr<Material> triangleMaterial = std::make_unique<Material>(
        engine.getRenderer()->getVulkanDevice(),
        engine.getRenderer()->getVmaAllocator(),
        engine.getRenderer()->getDescriptorLayouts()
    );
    
    // Create and setup texture
    std::shared_ptr<Texture> tex = engine.getRenderer()->createTexture("textures/white.jpg");
    int textureIndex = engine.getRenderer()->addTexture(tex);
    LOG_INFO("MAIN", "Added texture to array at index: {}", textureIndex);
    
    triangleMaterial->setDiffuseTexture(tex->imageView);
    triangleMaterial->setDefaultSampler(tex->sampler);
    
    // Initialize material's descriptor sets
    triangleMaterial->initializeDescriptorSets(engine.getRenderer()->getDescriptorAllocator());
    
    // Create pipeline
    std::shared_ptr<GraphicPipeline> trianglePipeline = std::make_shared<GraphicPipeline>(
        engine.getRenderer()->getVulkanDevice(),
        engine.getRenderer()->getRenderPass(),
        "shaders/triangle.vert.spv",
        "shaders/triangle.frag.spv",
        engine.getRenderer()->getSwapChainExtent(),
        &triangleMaterial->pipelineLayout
    );


    // Create mesh
    std::unique_ptr<Mesh> triangleMesh = std::make_unique<Mesh>(
        engine.getRenderer()->getVulkanDevice(),
        engine.getRenderer()->getVmaAllocator(),
        engine.getRenderer()->getCommandPool(),
        engine.getRenderer()->getGraphicsQueue(),
        meshData
    );
    
    triangleMaterial->pipeline = trianglePipeline;

    // xyz axis

    Mesh::MeshData axisMeshData;
    
    // lambda to add a box
    auto addBox = [&](float x, float y, float z, float sx, float sy, float sz, mathplease::Vector3 color) {
        uint32_t base = static_cast<uint32_t>(axisMeshData.vertices.size());
        
        // 8 vertices 
        struct P { float x,y,z; };
        P pts[] = {
            {x, y, z},          // 0
            {x+sx, y, z},       // 1
            {x+sx, y+sy, z},    // 2
            {x, y+sy, z},       // 3
            {x, y, z+sz},       // 4
            {x+sx, y, z+sz},    // 5
            {x+sx, y+sy, z+sz}, // 6
            {x, y+sy, z+sz}     // 7
        };
        
        for (const auto& pt : pts) {
             Vertex v{};
             v.pos = {pt.x, pt.y, pt.z};
             v.colour = color;
             v.normal = {0, 0, 1}; // Default
             v.uv = {0, 0};
             axisMeshData.vertices.push_back(v);
        }
        
        uint32_t idxs[] = {
            // Back (Z=0) - want -Z normal -> 0,2,1 ...
            base+0, base+2, base+1, base+0, base+3, base+2,
            // Front (Z=sz) - want +Z normal -> 4,5,6 ...
            base+4, base+5, base+6, base+4, base+6, base+7,
            // Left (X=0) - want -X normal -> 0,4,7 ...
            base+0, base+4, base+7, base+0, base+7, base+3,
            // Right (X=sx) - want +X normal -> 1,6,5 ...
            base+1, base+6, base+5, base+1, base+2, base+6,
            // Bottom (Y=0) - want -Y normal -> 0,1,5 ...
            base+0, base+1, base+5, base+0, base+5, base+4,
            // Top (Y=sy) - want +Y normal -> 3,6,2 ...
            base+3, base+6, base+2, base+3, base+7, base+6
        };
        
        for (uint32_t i : idxs) axisMeshData.indices.push_back(i);
    };

    float l = 2.0f; // Length
    float w = 0.05f; // Width

    // X Axis (Red)
    addBox(0,0,0, l, w, w, {1.0f, 0.0f, 0.0f});
    // Y Axis (Green)
    addBox(0,0,0, w, l, w, {0.0f, 1.0f, 0.0f});
    // Z Axis (Blue)
    addBox(0,0,0, w, w, l, {0.0f, 0.0f, 1.0f});

    std::unique_ptr<Mesh> axisMesh = std::make_unique<Mesh>(
        engine.getRenderer()->getVulkanDevice(),
        engine.getRenderer()->getVmaAllocator(),
        engine.getRenderer()->getCommandPool(),
        engine.getRenderer()->getGraphicsQueue(),
        axisMeshData
    );
    engine.getRenderer()->createDrawable(axisMesh.get(), triangleMaterial.get());

    
    
    // Create drawable
    // engine.getRenderer()->createDrawable(triangleMesh.get(), triangleMaterial.get());

    LOG_INFO("MAIN", "Application setup complete");


    EntityManager* entityManager = engine.getEntityManager();
    // create 10000 entities with Position and Velocity components
    ComponentMask mask = 0;
    mask |= Components::Position;
    mask |= Components::Velocity;
    mask |= Components::Gravity;
    for (int i = 0; i < 4000000; ++i) {

        Entity_id entityId = entityManager->createEntity(mask);
        // Position* pos = (Position*)entityManager->getComponentData(entityId, Components::Position);
        // Velocity* vel = (Velocity*)entityManager->getComponentData(entityId, Components::Velocity);
        // pos->value = mathplease::Vector3(i * 1.0f, 0.0f, 0.0f);
        // vel->value = mathplease::Vector3(0.0f, 0.1f, 0.0f);
    }


    for (int i = 0; i < 1000; ++i) {
        Entity_id entityId = i; // assuming entity IDs are assigned sequentially starting from 0
        entityManager->removeComponent(entityId, Components::Velocity);
    }
    for (int i = 1000; i < 1500; ++i) {
        Entity_id entityId = i; // assuming entity IDs are assigned sequentially starting from 0
        entityManager->addComponent(entityId, Components::Health);
    }

    
    // Run the engine
    engine.run();

    return 0;
}
