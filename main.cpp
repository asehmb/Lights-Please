
#include <SDL.h>
#include <vector>
#include "engine/engine.h"
#include "engine/geometry.h"
#include "engine/logger.h"
#include "engine/mesh.h"
#include "engine/material.hpp"
#include "engine/pipeline.h"
#include "engine/texture.h"
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
    std::shared_ptr<Texture> tex = engine.getRenderer()->createTexture("textures/pink.jpg");
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

    // traingle mesh 


    // Create mesh
    std::unique_ptr<Mesh> triangleMesh = std::make_unique<Mesh>(
        engine.getRenderer()->getVulkanDevice(),
        engine.getRenderer()->getVmaAllocator(),
        engine.getRenderer()->getCommandPool(),
        engine.getRenderer()->getGraphicsQueue(),
        meshData
    );
    
    triangleMaterial->pipeline = trianglePipeline;
    
    // Create drawable
    engine.getRenderer()->createDrawable(triangleMesh.get(), triangleMaterial.get());
    
    LOG_INFO("MAIN", "Application setup complete");
    
    // Run the engine
    engine.run();

    return 0;
}
