#include "demo_scene.h"

#include "engine.h"
#include "loadModel.h"
#include "logger.h"
#include <algorithm>
#include <cstdint>
#include <memory>
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

void DemoScene::renderAxisWithTriangle(Engine &engine) {
  Mesh::MeshData triangleData;
  const float halfSide = 0.5f;
  const float height = 0.8660254f; // sqrt(3) / 2

  triangleData.vertices = {
      {{0.0f, height * 0.5f, 0.0f},
       {1.0f, 0.0f, 0.0f},
       {0.0f, 0.0f, 1.0f},
       {0.5f, 0.0f}}, // top

      {{halfSide, -height * 0.5f, 0.0f},
       {0.0f, 1.0f, 0.0f},
       {0.0f, 0.0f, 1.0f},
       {1.0f, 1.0f}}, // bottom-right

      {{-halfSide, -height * 0.5f, 0.0f},
       {0.0f, 0.0f, 1.0f},
       {0.0f, 0.0f, 1.0f},
       {0.0f, 1.0f}} // bottom-left
  };

  triangleData.indices = {0, 1, 2};
  Renderer *renderer = engine.getRenderer();
  RenderSystem *renderSystem = engine.getRenderSystem();
  EntityManager *entityManager = engine.getEntityManager();
  SceneGraph *sceneGraph = engine.getSceneGraph();
  RuntimeAssetRegistry *assetRegistry = engine.getAssetRegistry();
  Camera *camera = engine.getCamera();

  assetRegistry->addMesh("triangle.mesh", renderer->createMesh(triangleData));

  uint32_t meshId =
      renderSystem->registerMesh(assetRegistry->getMesh("triangle.mesh").get());

  auto materialAsset = std::shared_ptr<Material>(renderer->createMaterial(
      "textures/white.jpg", "shaders/triangle.vert.spv",
      "shaders/triangle.frag.spv"));

  assetRegistry->addMaterial("triangle.material", materialAsset);

  uint32_t materialId = renderSystem->registerMaterial(
      assetRegistry->getMaterial("triangle.material").get());

  if (meshId == 0) {
    LOG_ERR("DEMO", "Failed to register triangle mesh");
  }
  if (materialId == 0) {
    LOG_ERR("DEMO", "Failed to register triangle material");
  }

  Entity_id entityId = entityManager->createEntity(Components::Position |
                                                   Components::Renderable |
                                                   Components::Transformable);

  Renderable *renderable = static_cast<Renderable *>(
      entityManager->getComponentData(entityId, Components::Renderable));
  renderable->meshId = meshId;
  renderable->materialId = materialId;

  Position *position = static_cast<Position *>(
      entityManager->getComponentData(entityId, Components::Position));
  position->value = mathplease::Vector4(-1.0f, 1.0f, 0.0f, 1.0f);

  Transform_id transformId = sceneGraph->createTransform(entityId);
  if (transformId == NULL_ENTITY) {
    throw std::runtime_error(
        "Failed to create transform for demo triangle entity");
  }

  Transformable *transformable = static_cast<Transformable *>(
      entityManager->getComponentData(entityId, Components::Transformable));
  transformable->handle = transformId;

  sceneGraph->setLocalPosition(transformId, position->value);

  sceneGraph->setLocalRotation(transformId,
                               mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  LOG_INFO("DEMO", "Rendered demo triangle with axis-aligned vertex colors");

  Mesh::MeshData axisData;

  const float axisLength = 2.0f;
  const float axisThickness = 0.025f;

  // Helper for adding a rectangular prism/cuboid
  auto addBox = [&](mathplease::Vector3 min, mathplease::Vector3 max,
                    mathplease::Vector3 color) {
    uint32_t start = static_cast<uint32_t>(axisData.vertices.size());

    mathplease::Vector3 normal(0.0f, 0.0f, 1.0f);

    axisData.vertices.push_back(
        {{min.x, min.y, min.z}, color, normal, {0.0f, 0.0f}});
    axisData.vertices.push_back(
        {{max.x, min.y, min.z}, color, normal, {1.0f, 0.0f}});
    axisData.vertices.push_back(
        {{max.x, max.y, min.z}, color, normal, {1.0f, 1.0f}});
    axisData.vertices.push_back(
        {{min.x, max.y, min.z}, color, normal, {0.0f, 1.0f}});

    axisData.vertices.push_back(
        {{min.x, min.y, max.z}, color, normal, {0.0f, 0.0f}});
    axisData.vertices.push_back(
        {{max.x, min.y, max.z}, color, normal, {1.0f, 0.0f}});
    axisData.vertices.push_back(
        {{max.x, max.y, max.z}, color, normal, {1.0f, 1.0f}});
    axisData.vertices.push_back(
        {{min.x, max.y, max.z}, color, normal, {0.0f, 1.0f}});

    std::vector<uint32_t> inds = {// back face
                                  0, 1, 2, 2, 3, 0,

                                  // front face
                                  4, 6, 5, 6, 4, 7,

                                  // left face
                                  0, 3, 7, 7, 4, 0,

                                  // right face
                                  1, 5, 6, 6, 2, 1,

                                  // top face
                                  3, 2, 6, 6, 7, 3,

                                  // bottom face
                                  0, 4, 5, 5, 1, 0};

    for (uint32_t i : inds) {
      axisData.indices.push_back(start + i);
    }
  };

  // X axis: red
  addBox(mathplease::Vector3(0.0f, -axisThickness, -axisThickness),
         mathplease::Vector3(-axisLength, axisThickness, axisThickness),
         mathplease::Vector3(1.0f, 0.0f, 0.0f));

  // Y axis: green, up
  addBox(mathplease::Vector3(-axisThickness, 0.0f, -axisThickness),
         mathplease::Vector3(axisThickness, axisLength, axisThickness),
         mathplease::Vector3(0.0f, 1.0f, 0.0f));

  // Z axis: blue
  addBox(mathplease::Vector3(-axisThickness, -axisThickness, 0.0f),
         mathplease::Vector3(axisThickness, axisThickness, -axisLength),
         mathplease::Vector3(0.0f, 0.0f, 1.0f));

  assetRegistry->addMesh("axis.mesh", renderer->createMesh(axisData));

  meshId =
      renderSystem->registerMesh(assetRegistry->getMesh("axis.mesh").get());

  materialAsset = std::shared_ptr<Material>(renderer->createMaterial(
      "textures/white.jpg", "shaders/triangle.vert.spv",
      "shaders/triangle.frag.spv"));

  assetRegistry->addMaterial("axis.material", materialAsset);

  materialId = renderSystem->registerMaterial(
      assetRegistry->getMaterial("axis.material").get());

  if (meshId == 0) {
    LOG_ERR("DEMO", "Failed to register axis mesh");
  }

  if (materialId == 0) {
    LOG_ERR("DEMO", "Failed to register axis material");
  }

  entityId = entityManager->createEntity(Components::Position |
                                         Components::Renderable |
                                         Components::Transformable);

  renderable = static_cast<Renderable *>(
      entityManager->getComponentData(entityId, Components::Renderable));

  renderable->meshId = meshId;
  renderable->materialId = materialId;

  position = static_cast<Position *>(
      entityManager->getComponentData(entityId, Components::Position));

  position->value = mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);

  transformId = sceneGraph->createTransform(entityId);

  if (transformId == NULL_ENTITY) {
    throw std::runtime_error("Failed to create transform for demo axis entity");
  }

  transformable = static_cast<Transformable *>(
      entityManager->getComponentData(entityId, Components::Transformable));

  transformable->handle = transformId;

  sceneGraph->setLocalPosition(transformId, position->value);

  sceneGraph->setLocalRotation(transformId,
                               mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
}
