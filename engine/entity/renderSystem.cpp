#include "renderSystem.h"
#include <algorithm>
#include <cstddef>
#include <string>

std::string RenderSystem::toAssetKey(uint32_t id) { return std::to_string(id); }

uint32_t RenderSystem::registerMesh(Mesh *mesh) {
  if (!mesh) {
    return 0;
  }

  auto existing = meshIdsByPtr.find(mesh);
  if (existing != meshIdsByPtr.end()) {
    return existing->second;
  }

  const uint32_t meshId = nextMeshId++;
  meshManager.Add(toAssetKey(meshId), mesh);
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
  materialManager.Add(toAssetKey(materialId), material);
  materialIdsByPtr.emplace(material, materialId);
  return materialId;
}

Mesh *RenderSystem::getMesh(uint32_t meshId) const {
  const std::string key = toAssetKey(meshId);
  if (!meshManager.Has(key)) {
    return nullptr;
  }
  Mesh **mesh = meshManager.Get(key);
  return mesh ? *mesh : nullptr;
}

Material *RenderSystem::getMaterial(uint32_t materialId) const {
  const std::string key = toAssetKey(materialId);
  if (!materialManager.Has(key)) {
    return nullptr;
  }
  Material **material = materialManager.Get(key);
  return material ? *material : nullptr;
}

Entity_id RenderSystem::createRenderableEntity(EntityManager &em, Mesh *mesh,
                                               Material *material,
                                               const mathplease::Vector4 &position) {
  const uint32_t meshId = registerMesh(mesh);
  const uint32_t materialId = registerMaterial(material);
  if (meshId == 0 || materialId == 0) {
    return NULL_ENTITY;
  }

  constexpr ComponentMask renderMask = Components::Position | Components::Renderable;
  Entity_id entityId = em.createEntity(renderMask);

  auto *entityPosition =
      static_cast<Position *>(em.getComponentData(entityId, Components::Position));
  auto *renderable =
      static_cast<Renderable *>(em.getComponentData(entityId, Components::Renderable));

  if (!entityPosition || !renderable) {
    return entityId;
  }

  entityPosition->value = position;
  renderable->meshId = meshId;
  renderable->materialId = materialId;
  return entityId;
}

std::vector<Renderer::Drawable>
RenderSystem::collectDrawables(EntityManager &em) const {
  std::vector<Renderer::Drawable> drawables;
  constexpr ComponentMask requiredComponents =
      Components::Renderable | Components::Position;

  const std::vector<Archetype *> archetypes =
      em.getAllArchetypesWithComponent(requiredComponents);

  const uint8_t renderableIndex = componentMaskToIndex(Components::Renderable);
  const uint8_t positionIndex = componentMaskToIndex(Components::Position);

  for (Archetype *archetype : archetypes) {
    if (!archetype) {
      continue;
    }

    const uint32_t renderableOffset = archetype->offsets[renderableIndex];
    const uint32_t positionOffset = archetype->offsets[positionIndex];

    for (Chunk *chunk : archetype->chunks) {
      if (!chunk || chunk->row == 0) {
        continue;
      }

      auto *chunkData = static_cast<std::byte *>(chunk->data);
      auto *renderables =
          reinterpret_cast<Renderable *>(chunkData + renderableOffset);
      auto *positions = reinterpret_cast<Position *>(chunkData + positionOffset);

      for (uint32_t i = 0; i < chunk->row; ++i) {
        Mesh *mesh = getMesh(renderables[i].meshId);
        Material *material = getMaterial(renderables[i].materialId);
        if (!mesh || !material) {
          continue;
        }

        Renderer::Drawable drawable{mesh, material};
        drawable.transform =
            mathplease::Matrix4::translate(positions[i].value.xyz());
        drawables.push_back(drawable);
      }
    }
  }

  std::sort(drawables.begin(), drawables.end(),
            [](const Renderer::Drawable &a, const Renderer::Drawable &b) {
              if (a.material != b.material) {
                return a.material < b.material;
              }
              return a.mesh < b.mesh;
            });

  return drawables;
}

void RenderSystem::update(EntityManager &em, Renderer &renderer) {
  renderer.clearDrawables();
  const auto drawables = collectDrawables(em);
  for (const auto &drawable : drawables) {
    renderer.addDrawable(drawable);
  }
}
