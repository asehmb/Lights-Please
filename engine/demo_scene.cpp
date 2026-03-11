#include "demo_scene.h"

#include "loadModel.h"
#include "logger.h"
#include <stdexcept>

namespace {

constexpr const char *kWhiteTexturePath = "textures/white.jpg";
constexpr const char *kVertexShaderPath = "shaders/triangle.vert.spv";
constexpr const char *kFragmentShaderPath = "shaders/triangle.frag.spv";
constexpr const char *kMinionModelPath = "models/Minion.obj";
constexpr const char *kAxisMeshAssetId = "mesh.axis";
constexpr const char *kMinionMeshAssetId = "mesh.minion";
constexpr const char *kDefaultMaterialAssetId = "material.default";

Mesh::MeshData loadObjMeshData(const char *modelPath) {
  Mesh::MeshData meshData;
  modelsPlease::loadModelFromOBJ(modelPath, meshData.vertices, meshData.indices);
  return meshData;
}

void spawnRenderable(RenderSystem &renderSystem, EntityManager &entityManager,
                     Mesh &mesh, Material &material,
                     const mathplease::Vector4 &position) {
  renderSystem.createRenderableEntity(entityManager, &mesh, &material, position);
}

} // namespace

void DemoScene::load(Engine &engine) {
  Renderer *renderer = engine.getRenderer();
  RenderSystem *renderSystem = engine.getRenderSystem();
  EntityManager *entityManager = engine.getEntityManager();
  RuntimeAssetRegistry *assetRegistry = engine.getAssetRegistry();

  if (!renderer || !renderSystem || !entityManager || !assetRegistry) {
    throw std::runtime_error("Engine subsystems are not initialized");
  }

  auto sharedMaterial = assetRegistry->getMaterial(kDefaultMaterialAssetId);
  if (!sharedMaterial) {
    sharedMaterial = std::shared_ptr<Material>(renderer->createMaterial(
        kWhiteTexturePath, kVertexShaderPath, kFragmentShaderPath));
    assetRegistry->addMaterial(kDefaultMaterialAssetId, sharedMaterial);
  }

  auto axisMesh = assetRegistry->getMesh(kAxisMeshAssetId);
  if (!axisMesh) {
    axisMesh = std::shared_ptr<Mesh>(renderer->createAxisMesh());
    assetRegistry->addMesh(kAxisMeshAssetId, axisMesh);
  }
  spawnRenderable(*renderSystem, *entityManager, *axisMesh, *sharedMaterial,
                  mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  auto minionMesh = assetRegistry->getMesh(kMinionMeshAssetId);
  if (!minionMesh) {
    const Mesh::MeshData minionMeshData = loadObjMeshData(kMinionModelPath);
    minionMesh = std::shared_ptr<Mesh>(renderer->createMesh(minionMeshData));
    assetRegistry->addMesh(kMinionMeshAssetId, minionMesh);
  }
  spawnRenderable(*renderSystem, *entityManager, *minionMesh, *sharedMaterial,
                  mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  LOG_INFO("MAIN", "Loaded demo scene from asset registry: {}, {}",
           kAxisMeshAssetId, kMinionMeshAssetId);
}
