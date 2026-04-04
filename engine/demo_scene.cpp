#include "demo_scene.h"

#include "loadModel.h"
#include "logger.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace {

constexpr const char *kWhiteTexturePath = "textures/white.jpg";
constexpr const char *kVertexShaderPath = "shaders/triangle.vert.spv";
constexpr const char *kFragmentShaderPath = "shaders/triangle.frag.spv";
constexpr const char *kMinionModelPath = "models/Minion.obj";
constexpr const char *kMinionMeshAssetId = "mesh.minion";
constexpr const char *kDefaultMaterialAssetId = "material.default";
constexpr std::uint32_t kStressEntityCount = 10'000u;
constexpr std::uint32_t kStressLogInterval = 1'000u;
constexpr std::uint32_t kStressGridWidth = 2048u;
constexpr float kStressEntitySpacing = 2.0f;
constexpr std::uint32_t kVisibleEntityCount = 256u;
constexpr std::uint32_t kVisibleGridWidth = 128u;
constexpr float kVisibleEntitySpacing = 1.25f;
constexpr float kVisibleBandStart = 8.0f;
constexpr float kVisibleBandHeight = 1.5f;
constexpr ComponentMask kStressComponentMask =
    Components::Position | Components::Velocity | Components::Health |
    Components::Renderable | Components::AI | Components::Transformable |
    Components::RigidBody | Components::Collider | Components::PhysicsMaterial;

Mesh::MeshData loadObjMeshData(const char *modelPath) {
  Mesh::MeshData meshData;
  modelsPlease::loadModelFromOBJ(modelPath, meshData.vertices,
                                 meshData.indices);
  return meshData;
}

void initializeStressEntity(EntityManager &entityManager,
                            SceneGraph &sceneGraph, Entity_id entityId,
                            std::uint32_t index, std::uint32_t visibleIndex,
                            bool visible, std::uint32_t meshId,
                            std::uint32_t materialId) {
  auto *position = static_cast<Position *>(
      entityManager.getComponentData(entityId, Components::Position));
  auto *velocity = static_cast<Velocity *>(
      entityManager.getComponentData(entityId, Components::Velocity));
  auto *health = static_cast<Health *>(
      entityManager.getComponentData(entityId, Components::Health));
  auto *renderable = static_cast<Renderable *>(
      entityManager.getComponentData(entityId, Components::Renderable));
  auto *ai = static_cast<AI *>(
      entityManager.getComponentData(entityId, Components::AI));
  auto *transformable = static_cast<Transformable *>(
      entityManager.getComponentData(entityId, Components::Transformable));
  auto *rigidBody = static_cast<RigidBody *>(
      entityManager.getComponentData(entityId, Components::RigidBody));
  auto *collider = static_cast<Collider *>(
      entityManager.getComponentData(entityId, Components::Collider));
  auto *physicsMaterial = static_cast<PhysicsMaterial *>(
      entityManager.getComponentData(entityId, Components::PhysicsMaterial));

  if (!position || !velocity || !health || !renderable || !ai ||
      !transformable || !rigidBody || !collider || !physicsMaterial) {
    throw std::runtime_error("Failed to initialize stress entity components");
  }

  const std::uint32_t gridX = index % kStressGridWidth;
  const std::uint32_t gridZ = index / kStressGridWidth;
  const float x = static_cast<float>(gridX) * kStressEntitySpacing;
  const float z = static_cast<float>(gridZ) * kStressEntitySpacing;

  position->value = mathplease::Vector4(x, 10.0f, z, 1.0f);
  velocity->value = mathplease::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

  health->current = 100;
  health->max = 100;

  renderable->meshId = meshId;
  renderable->materialId = materialId;

  ai->state = static_cast<std::uint8_t>(index & 0x3u);
  ai->type = static_cast<std::uint8_t>((index / 4u) & 0x7u);
  ai->aggressionLevel = 0.5f;

  transformable->handle = NULL_ENTITY;
  if (visible) {
    const Transform_id transformId = sceneGraph.createTransform(entityId);
    if (transformId == NULL_ENTITY) {
      throw std::runtime_error(
          "Failed to create transform for visible stress-test entity");
    }

    const std::uint32_t visibleX = visibleIndex % kVisibleGridWidth;
    const std::uint32_t visibleZ = visibleIndex / kVisibleGridWidth;
    position->value =
        mathplease::Vector4(kVisibleBandStart + static_cast<float>(visibleX) *
                                                    kVisibleEntitySpacing,
                            kVisibleBandHeight,
                            kVisibleBandStart + static_cast<float>(visibleZ) *
                                                    kVisibleEntitySpacing,
                            1.0f);
    transformable->handle = transformId;
    sceneGraph.setLocalPosition(transformId, position->value);
  }

  rigidBody->angularVelocity = mathplease::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
  rigidBody->mass = 1.0f;
  rigidBody->inverseMass = 1.0f;
  rigidBody->damping = 0.98f;
  rigidBody->type = RigidBodyType::Dynamic;

  collider->offset = mathplease::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
  collider->collisionLayer = CollisionLayers::Default;
  collider->collisionMask = CollisionLayers::All;
  collider->behaviorFlags = ColliderBehavior::Solid;
  collider->shape = ColliderShape::Box;
  collider->reserved[0] = 0;
  collider->reserved[1] = 0;
  collider->reserved[2] = 0;
  collider->halfExtents = mathplease::Vector4(0.5f, 0.5f, 0.5f, 0.0f);

  physicsMaterial->friction = 0.5f;
  physicsMaterial->restitution = 0.1f;
}

} // namespace

void DemoScene::load(Engine &engine) {
  Renderer *renderer = engine.getRenderer();
  RenderSystem *renderSystem = engine.getRenderSystem();
  EntityManager *entityManager = engine.getEntityManager();
  SceneGraph *sceneGraph = engine.getSceneGraph();
  RuntimeAssetRegistry *assetRegistry = engine.getAssetRegistry();

  if (!renderer || !renderSystem || !entityManager || !sceneGraph ||
      !assetRegistry) {
    throw std::runtime_error("Engine subsystems are not initialized");
  }

  auto sharedMaterial = assetRegistry->getMaterial(kDefaultMaterialAssetId);
  if (!sharedMaterial) {
    sharedMaterial = std::shared_ptr<Material>(renderer->createMaterial(
        kWhiteTexturePath, kVertexShaderPath, kFragmentShaderPath));
    assetRegistry->addMaterial(kDefaultMaterialAssetId, sharedMaterial);
  }

  auto minionMesh = assetRegistry->getMesh(kMinionMeshAssetId);
  if (!minionMesh) {
    const Mesh::MeshData minionMeshData = loadObjMeshData(kMinionModelPath);
    minionMesh = std::shared_ptr<Mesh>(renderer->createMesh(minionMeshData));
    assetRegistry->addMesh(kMinionMeshAssetId, minionMesh);
  }

  const std::uint32_t meshId = renderSystem->registerMesh(minionMesh.get());
  const std::uint32_t materialId =
      renderSystem->registerMaterial(sharedMaterial.get());
  if (meshId == 0 || materialId == 0) {
    throw std::runtime_error("Failed to register stress-test render assets");
  }

  const std::uint32_t visibleEntityCount =
      std::min(std::min(kStressEntityCount, kVisibleEntityCount),
               static_cast<std::uint32_t>(MAX_TRANSFORMS));

  for (std::uint32_t index = 0; index < kStressEntityCount; ++index) {
    const Entity_id entityId =
        entityManager->createEntity(kStressComponentMask);
    if (entityId == NULL_ENTITY) {
      throw std::runtime_error(
          "Failed to create stress-test entity during demo scene startup");
    }
    const bool visible = index < visibleEntityCount;
    const std::uint32_t visibleIndex = visible ? index : 0u;
    initializeStressEntity(*entityManager, *sceneGraph, entityId, index,
                           visibleIndex, visible, meshId, materialId);

    if ((index + 1) % kStressLogInterval == 0) {
      LOG_INFO("MAIN", "Spawned {} / {} stress-test entities", index + 1,
               kStressEntityCount);
    }
  }

  LOG_INFO("MAIN",
           "Loaded demo stress scene with {} entities ({} visible render "
           "transforms)",
           kStressEntityCount, visibleEntityCount);
}
