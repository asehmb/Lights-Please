#pragma once

#include "../renderer/renderer.h"
#include "AssetManager.h"
#include "entity.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class RenderSystem {
public:
  RenderSystem() = default;
  RenderSystem(const RenderSystem &) = delete;
  RenderSystem &operator=(const RenderSystem &) = delete;

  uint32_t registerMesh(Mesh *mesh);
  uint32_t registerMaterial(Material *material);
  Mesh *getMesh(uint32_t meshId) const;
  Material *getMaterial(uint32_t materialId) const;
  Entity_id createRenderableEntity(EntityManager &em, Mesh *mesh,
                                   Material *material,
                                   const mathplease::Vector4 &position);

  std::vector<Renderer::Drawable> collectDrawables(EntityManager &em) const;
  void update(EntityManager &em, Renderer &renderer);

private:
  static std::string toAssetKey(uint32_t id);

  mutable AssetManager<Mesh *> meshManager;
  mutable AssetManager<Material *> materialManager;
  std::unordered_map<const Mesh *, uint32_t> meshIdsByPtr;
  std::unordered_map<const Material *, uint32_t> materialIdsByPtr;
  uint32_t nextMeshId = 1;
  uint32_t nextMaterialId = 1;
};
