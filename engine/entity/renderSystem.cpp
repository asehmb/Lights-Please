#include "renderSystem.h"
#include <algorithm>
#include <cstddef>

namespace {
float clamp01(float value) { return std::clamp(value, 0.0f, 1.0f); }

mathplease::Matrix4 lerpMatrix4(const mathplease::Matrix4 &a,
                                const mathplease::Matrix4 &b, float t) {
  return a + (b - a) * t;
}
} // namespace

bool RenderSystem::drawableLess(const Renderer::Drawable &a,
                                const Renderer::Drawable &b) {
  if (a.material != b.material) {
    return a.material < b.material;
  }
  return a.mesh < b.mesh;
}

uint32_t RenderSystem::registerMesh(Mesh *mesh) {
  if (!mesh) {
    return 0;
  }

  auto existing = meshIdsByPtr.find(mesh);
  if (existing != meshIdsByPtr.end()) {
    return existing->second;
  }

  const uint32_t meshId = nextMeshId++;
  if (meshesById.size() <= meshId) {
    meshesById.resize(static_cast<size_t>(meshId) + 1, nullptr);
  }
  meshesById[meshId] = mesh;
  meshIdsByPtr.emplace(mesh, meshId);
  return meshId;
}

uint32_t RenderSystem::registerMaterial(Material *material) {
  if (!material) {
    return 0;
  }

  auto existing = materialIdsByPtr.find(material);
  if (existing != materialIdsByPtr.end()) {
    return existing->second;
  }

  const uint32_t materialId = nextMaterialId++;
  if (materialsById.size() <= materialId) {
    materialsById.resize(static_cast<size_t>(materialId) + 1, nullptr);
  }
  materialsById[materialId] = material;
  materialIdsByPtr.emplace(material, materialId);
  return materialId;
}

Mesh *RenderSystem::getMesh(uint32_t meshId) const {
  if (meshId >= meshesById.size()) {
    return nullptr;
  }
  return meshesById[meshId];
}

Material *RenderSystem::getMaterial(uint32_t materialId) const {
  if (materialId >= materialsById.size()) {
    return nullptr;
  }
  return materialsById[materialId];
}

void RenderSystem::capturePreviousState(EntityManager &em, SceneGraph &sceneGraph) {
  std::unordered_map<Entity_id, mathplease::Matrix4> nextPreviousWorldMatrices;
  constexpr ComponentMask requiredComponents =
      Components::Renderable | Components::Transformable;

  const auto &archetypes = em.getAllArchetypesWithComponent(requiredComponents);
  nextPreviousWorldMatrices.reserve(previousWorldMatricesByEntity.size());

  const uint8_t transformableIndex = componentMaskToIndex(Components::Transformable);
  for (Archetype *archetype : archetypes) {
    if (!archetype) {
      continue;
    }

    const uint32_t transformableOffset = archetype->offsets[transformableIndex];
    for (Chunk *chunk : archetype->chunks) {
      if (!chunk || chunk->row == 0) {
        continue;
      }

      auto *chunkData = static_cast<std::byte *>(chunk->data);
      auto *entityIds = reinterpret_cast<Entity_id *>(chunkData);
      auto *transformables =
          reinterpret_cast<Transformable *>(chunkData + transformableOffset);
      for (uint32_t i = 0; i < chunk->row; ++i) {
        mathplease::Matrix4 *world =
            sceneGraph.getWorldMatrixPtr(transformables[i].handle);
        if (world) {
          nextPreviousWorldMatrices[entityIds[i]] = *world;
        }
      }
    }
  }

  previousWorldMatricesByEntity = std::move(nextPreviousWorldMatrices);
}

Entity_id RenderSystem::createRenderableEntity(EntityManager &em,
                                               SceneGraph &sceneGraph, Mesh *mesh,
                                               Material *material,
                                               const mathplease::Vector4 &position) {
  const uint32_t meshId = registerMesh(mesh);
  const uint32_t materialId = registerMaterial(material);
  if (meshId == 0 || materialId == 0) {
    return NULL_ENTITY;
  }

  constexpr ComponentMask renderMask =
      Components::Transformable | Components::Renderable;
  Entity_id entityId = em.createEntity(renderMask);

  auto *transformable = static_cast<Transformable *>(
      em.getComponentData(entityId, Components::Transformable));
  auto *renderable =
      static_cast<Renderable *>(em.getComponentData(entityId, Components::Renderable));

  if (!transformable || !renderable) {
    return entityId;
  }

  transformable->handle = NULL_ENTITY;
  Transform_id handle = sceneGraph.createTransform(entityId);
  if (handle == NULL_ENTITY) {
    em.destroyEntity(entityId);
    return NULL_ENTITY;
  }
  transformable->handle = handle;
  sceneGraph.setLocalPosition(handle, position);
  renderable->meshId = meshId;
  renderable->materialId = materialId;
  return entityId;
}

void RenderSystem::collectDrawablesInto(EntityManager &em, SceneGraph &sceneGraph,
                                        float alpha,
                                        std::vector<Renderer::Drawable> &out) const {
  out.clear();
  constexpr ComponentMask requiredComponents =
      Components::Renderable | Components::Transformable;
  const float interpolationAlpha = clamp01(alpha);
  const bool hasPreviousWorldState = !previousWorldMatricesByEntity.empty();
  bool requiresSort = false;
  Material *firstMaterial = nullptr;
  Mesh *firstMesh = nullptr;
  bool hasFirstKey = false;

  const auto &archetypes = em.getAllArchetypesWithComponent(requiredComponents);

  const uint8_t renderableIndex = componentMaskToIndex(Components::Renderable);
  const uint8_t transformableIndex = componentMaskToIndex(Components::Transformable);

  for (Archetype *archetype : archetypes) {
    if (!archetype) {
      continue;
    }

    const uint32_t renderableOffset = archetype->offsets[renderableIndex];
    const uint32_t transformableOffset = archetype->offsets[transformableIndex];

    for (Chunk *chunk : archetype->chunks) {
      if (!chunk || chunk->row == 0) {
        continue;
      }

      auto *chunkData = static_cast<std::byte *>(chunk->data);
      auto *entityIds = reinterpret_cast<Entity_id *>(chunkData);
      auto *renderables =
          reinterpret_cast<Renderable *>(chunkData + renderableOffset);
      auto *transformables =
          reinterpret_cast<Transformable *>(chunkData + transformableOffset);

      for (uint32_t i = 0; i < chunk->row; ++i) {
        Mesh *mesh = getMesh(renderables[i].meshId);
        Material *material = getMaterial(renderables[i].materialId);
        if (!mesh || !material) {
          continue;
        }

        mathplease::Matrix4 *currentWorld =
            sceneGraph.getWorldMatrixPtr(transformables[i].handle);
        if (!currentWorld) {
          continue;
        }

        mathplease::Matrix4 interpolatedWorld = *currentWorld;
        if (hasPreviousWorldState) {
          const auto previousWorldIt = previousWorldMatricesByEntity.find(entityIds[i]);
          if (previousWorldIt != previousWorldMatricesByEntity.end()) {
            interpolatedWorld = lerpMatrix4(previousWorldIt->second, *currentWorld,
                                            interpolationAlpha);
          }
        }

        Renderer::Drawable drawable{mesh, material};
        drawable.transform = interpolatedWorld;
        out.push_back(drawable);
        if (!hasFirstKey) {
          hasFirstKey = true;
          firstMaterial = material;
          firstMesh = mesh;
        } else if (!requiresSort &&
                   (material != firstMaterial || mesh != firstMesh)) {
          requiresSort = true;
        }
      }
    }
  }

  if (requiresSort) {
    std::sort(out.begin(), out.end(), drawableLess);
  }
}

std::vector<Renderer::Drawable>
RenderSystem::collectDrawables(EntityManager &em, SceneGraph &sceneGraph,
                               float alpha) const {
  std::vector<Renderer::Drawable> drawables;
  collectDrawablesInto(em, sceneGraph, alpha, drawables);
  return drawables;
}

void RenderSystem::update(EntityManager &em, SceneGraph &sceneGraph,
                          Renderer &renderer, float alpha) {
  renderer.setPhysicsDebugOverlayState(collectPhysicsDebugOverlayState(em));
  collectDrawablesInto(em, sceneGraph, alpha, frameDrawablesScratch);
  renderer.swapDrawables(frameDrawablesScratch);
}

void RenderSystem::setPhysicsDebugVisualizationSettings(
    const PhysicsDebugVisualizationSettings &settings) {
  physicsDebugVisualizationSettings = settings;
}

void RenderSystem::toggleColliderWireframes() {
  physicsDebugVisualizationSettings.showColliderWireframes =
      !physicsDebugVisualizationSettings.showColliderWireframes;
}

void RenderSystem::toggleAABBs() {
  physicsDebugVisualizationSettings.showAABBs =
      !physicsDebugVisualizationSettings.showAABBs;
}

void RenderSystem::toggleContactPoints() {
  physicsDebugVisualizationSettings.showContactPoints =
      !physicsDebugVisualizationSettings.showContactPoints;
}

Renderer::PhysicsDebugOverlayState
RenderSystem::collectPhysicsDebugOverlayState(EntityManager &em) const {
  Renderer::PhysicsDebugOverlayState state{};
  state.showColliderWireframes =
      physicsDebugVisualizationSettings.showColliderWireframes;
  state.showAABBs = physicsDebugVisualizationSettings.showAABBs;
  state.showContactPoints = physicsDebugVisualizationSettings.showContactPoints;

  if (!state.showColliderWireframes && !state.showAABBs &&
      !state.showContactPoints) {
    return state;
  }

  constexpr ComponentMask requiredComponents =
      Components::Transformable | Components::Collider;
  const auto &archetypes = em.getAllArchetypesWithComponent(requiredComponents);

  const uint8_t transformableIndex = componentMaskToIndex(Components::Transformable);
  const uint8_t colliderIndex = componentMaskToIndex(Components::Collider);

  for (Archetype *archetype : archetypes) {
    if (!archetype) {
      continue;
    }

    const uint32_t transformableOffset = archetype->offsets[transformableIndex];
    const uint32_t colliderOffset = archetype->offsets[colliderIndex];
    for (Chunk *chunk : archetype->chunks) {
      if (!chunk || chunk->row == 0) {
        continue;
      }

      auto *chunkData = static_cast<std::byte *>(chunk->data);
      auto *transformables =
          reinterpret_cast<Transformable *>(chunkData + transformableOffset);
      auto *colliders = reinterpret_cast<Collider *>(chunkData + colliderOffset);
      for (uint32_t i = 0; i < chunk->row; ++i) {
        if (transformables[i].handle == NULL_ENTITY) {
          continue;
        }
        (void)colliders;
        if (state.showColliderWireframes) {
          state.colliderCount++;
        }
        if (state.showAABBs) {
          state.aabbCount++;
        }
      }
    }
  }

  // Placeholder until contact generation exists.
  state.contactPointCount = 0;
  return state;
}
