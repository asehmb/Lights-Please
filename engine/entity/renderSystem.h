#pragma once

#include "../renderer/renderer.h"
#include "entity.h"
#include "sceneGraph.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

class RenderSystem {
public:
  struct PhysicsDebugVisualizationSettings {
    bool showColliderWireframes = false;
    bool showAABBs = false;
    bool showContactPoints = false;
  };

  RenderSystem() = default;
  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  uint32_t registerMesh(Mesh *mesh);
  uint32_t registerMaterial(Material *material);
  Mesh *getMesh(uint32_t meshId) const;
  Material *getMaterial(uint32_t materialId) const;
  void capturePreviousState(EntityManager &em, SceneGraph &sceneGraph);
  Entity_id createRenderableEntity(EntityManager &em, SceneGraph &sceneGraph,
                                   Mesh *mesh,
                                   Material *material,
                                   const mathplease::Vector4 &position);

  std::vector<Renderer::Drawable> collectDrawables(EntityManager &em,
                                                   SceneGraph &sceneGraph,
                                                   float alpha = 1.0f) const;
  void update(EntityManager &em, SceneGraph &sceneGraph, Renderer &renderer,
              float alpha = 1.0f);
  void setPhysicsDebugVisualizationSettings(
      const PhysicsDebugVisualizationSettings &settings);
  const PhysicsDebugVisualizationSettings &
  getPhysicsDebugVisualizationSettings() const {
    return physicsDebugVisualizationSettings;
  }
  void toggleColliderWireframes();
  void toggleAABBs();
  void toggleContactPoints();

private:
  static bool drawableLess(const Renderer::Drawable &a,
                           const Renderer::Drawable &b);
  void collectDrawablesInto(EntityManager &em, SceneGraph &sceneGraph, float alpha,
                            std::vector<Renderer::Drawable> &out) const;
  Renderer::PhysicsDebugOverlayState
  collectPhysicsDebugOverlayState(EntityManager &em) const;

  std::vector<Mesh *> meshesById;
  std::vector<Material *> materialsById;
  std::unordered_map<const Mesh *, uint32_t> meshIdsByPtr;
  std::unordered_map<const Material *, uint32_t> materialIdsByPtr;
  std::unordered_map<Entity_id, mathplease::Matrix4> previousWorldMatricesByEntity;
  std::vector<Renderer::Drawable> frameDrawablesScratch;
  PhysicsDebugVisualizationSettings physicsDebugVisualizationSettings;
  uint32_t nextMeshId = 1;
  uint32_t nextMaterialId = 1;
};
