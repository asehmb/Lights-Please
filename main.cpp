#include "engine/engine.h"
#include "engine/logger.h"
#include <cstdint>
#include <memory>

#define TINYOBJLOADER_IMPLEMENTATION
#include "engine/loadModel.h"

int main() {
  Engine engine;
  engine.initialize();

  Renderer *renderer = engine.getRenderer();
  RenderSystem *renderSystem = engine.getRenderSystem();
  EntityManager *entityManager = engine.getEntityManager();

  Mesh::MeshData meshData;
  modelsPlease::loadModelFromOBJ("models/Minion.obj", meshData.vertices,
                                 meshData.indices);

  std::unique_ptr<Material> defaultMaterial = renderer->createMaterial(
      "textures/white.jpg", "shaders/triangle.vert.spv",
      "shaders/triangle.frag.spv");
  std::unique_ptr<Mesh> modelMesh = renderer->createMesh(meshData);
  std::unique_ptr<Mesh> axisMesh = renderer->createAxisMesh();

  renderSystem->createRenderableEntity(
      *entityManager, axisMesh.get(), defaultMaterial.get(),
      mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
  renderSystem->createRenderableEntity(
      *entityManager, modelMesh.get(), defaultMaterial.get(),
      mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  LOG_INFO("MAIN", "Application setup complete");

  ComponentMask mask =
      Components::Position | Components::Velocity | Components::Gravity;
  for (int i = 0; i < 40000; ++i) {
    entityManager->createEntity(mask);
  }

  for (int i = 0; i < 1000; ++i) {
    entityManager->removeComponent(i, Components::Velocity);
  }

  for (int i = 1000; i < 1500; ++i) {
    entityManager->addComponent(i, Components::Health);
  }

  engine.run();
  return 0;
}
