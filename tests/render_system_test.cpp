#include "../engine/entity/renderSystem.h"
#include <cassert>
#include <cstddef>

int main() {
  EntityManager em;
  RenderSystem renderSystem;

  alignas(Mesh) std::byte meshStorage[sizeof(Mesh)];
  alignas(Mesh) std::byte secondMeshStorage[sizeof(Mesh)];
  alignas(Material) std::byte materialStorage[sizeof(Material)];
  Mesh *mesh = reinterpret_cast<Mesh *>(meshStorage);
  Mesh *secondMesh = reinterpret_cast<Mesh *>(secondMeshStorage);
  Material *material = reinterpret_cast<Material *>(materialStorage);

  const uint32_t meshId = renderSystem.registerMesh(mesh);
  const uint32_t meshIdAgain = renderSystem.registerMesh(mesh);
  const uint32_t secondMeshId = renderSystem.registerMesh(secondMesh);
  const uint32_t materialId = renderSystem.registerMaterial(material);
  const uint32_t materialIdAgain = renderSystem.registerMaterial(material);

  assert(meshId != 0);
  assert(secondMeshId != 0);
  assert(materialId != 0);
  assert(meshId == meshIdAgain);
  assert(materialId == materialIdAgain);
  assert(meshId != secondMeshId);

  assert(renderSystem.getMesh(meshId) == mesh);
  assert(renderSystem.getMesh(secondMeshId) == secondMesh);
  assert(renderSystem.getMaterial(materialId) == material);
  assert(renderSystem.getMesh(999) == nullptr);
  assert(renderSystem.getMaterial(999) == nullptr);

  Entity_id firstEntity = renderSystem.createRenderableEntity(
      em, mesh, material, mathplease::Vector4(1.5f, -2.0f, 3.25f, 1.0f));
  Entity_id secondEntity = renderSystem.createRenderableEntity(
      em, secondMesh, material, mathplease::Vector4(0.0f, 1.0f, 2.0f, 1.0f));
  Entity_id invalidEntity = renderSystem.createRenderableEntity(
      em, nullptr, material, mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  assert(firstEntity != NULL_ENTITY);
  assert(secondEntity != NULL_ENTITY);
  assert(invalidEntity == NULL_ENTITY);

  const auto drawables = renderSystem.collectDrawables(em);
  assert(drawables.size() == 2);
  const Renderer::Drawable *firstDrawable = nullptr;
  const Renderer::Drawable *secondDrawable = nullptr;
  for (const auto &drawable : drawables) {
    if (drawable.mesh == mesh) {
      firstDrawable = &drawable;
    } else if (drawable.mesh == secondMesh) {
      secondDrawable = &drawable;
    }
  }

  assert(firstDrawable != nullptr);
  assert(secondDrawable != nullptr);
  assert(firstDrawable->material == material);
  assert(firstDrawable->transform(0, 3) == 1.5f);
  assert(firstDrawable->transform(1, 3) == -2.0f);
  assert(firstDrawable->transform(2, 3) == 3.25f);
  assert(secondDrawable->material == material);
  assert(secondDrawable->transform(0, 3) == 0.0f);
  assert(secondDrawable->transform(1, 3) == 1.0f);
  assert(secondDrawable->transform(2, 3) == 2.0f);

  return 0;
}
