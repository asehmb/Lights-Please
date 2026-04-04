#include "../engine/entity/renderSystem.h"
#include <cassert>
#include <cmath>
#include <cstddef>

int main() {
  EntityManager em;
  SceneGraph sceneGraph(em);
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
      em, sceneGraph, mesh, material,
      mathplease::Vector4(1.5f, -2.0f, 3.25f, 1.0f));
  Entity_id secondEntity = renderSystem.createRenderableEntity(
      em, sceneGraph, secondMesh, material,
      mathplease::Vector4(0.0f, 1.0f, 2.0f, 1.0f));
  Entity_id invalidEntity = renderSystem.createRenderableEntity(
      em, sceneGraph, nullptr, material,
      mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));

  assert(firstEntity != NULL_ENTITY);
  assert(secondEntity != NULL_ENTITY);
  assert(invalidEntity == NULL_ENTITY);

  sceneGraph.updateWorldTransforms(nullptr);
  const auto drawables = renderSystem.collectDrawables(em, sceneGraph);
  assert(drawables.size() == 2);

  auto debugSettings = renderSystem.getPhysicsDebugVisualizationSettings();
  assert(!debugSettings.showColliderWireframes);
  assert(!debugSettings.showAABBs);
  assert(!debugSettings.showContactPoints);
  renderSystem.toggleColliderWireframes();
  renderSystem.toggleAABBs();
  renderSystem.toggleContactPoints();
  debugSettings = renderSystem.getPhysicsDebugVisualizationSettings();
  assert(debugSettings.showColliderWireframes);
  assert(debugSettings.showAABBs);
  assert(debugSettings.showContactPoints);
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

  renderSystem.capturePreviousState(em, sceneGraph);
  auto *firstTransformable = static_cast<Transformable *>(
      em.getComponentData(firstEntity, Components::Transformable));
  assert(firstTransformable != nullptr);
  sceneGraph.setLocalPosition(firstTransformable->handle,
                              mathplease::Vector4(5.5f, -2.0f, 3.25f, 1.0f));

  const auto interpolatedDrawables =
      renderSystem.collectDrawables(em, sceneGraph, 0.25f);
  const Renderer::Drawable *interpolatedFirstDrawable = nullptr;
  for (const auto &drawable : interpolatedDrawables) {
    if (drawable.mesh == mesh) {
      interpolatedFirstDrawable = &drawable;
      break;
    }
  }
  assert(interpolatedFirstDrawable != nullptr);
  assert(std::fabs(interpolatedFirstDrawable->transform(0, 3) - 2.5f) < 1e-5f);
  assert(std::fabs(interpolatedFirstDrawable->transform(1, 3) + 2.0f) < 1e-5f);
  assert(std::fabs(interpolatedFirstDrawable->transform(2, 3) - 3.25f) < 1e-5f);

  const auto clampedDrawables = renderSystem.collectDrawables(em, sceneGraph, 1.5f);
  const Renderer::Drawable *clampedFirstDrawable = nullptr;
  for (const auto &drawable : clampedDrawables) {
    if (drawable.mesh == mesh) {
      clampedFirstDrawable = &drawable;
      break;
    }
  }
  assert(clampedFirstDrawable != nullptr);
  assert(std::fabs(clampedFirstDrawable->transform(0, 3) - 5.5f) < 1e-5f);

  Entity_id lateEntity = renderSystem.createRenderableEntity(
      em, sceneGraph, mesh, material,
      mathplease::Vector4(9.0f, 0.0f, 0.0f, 1.0f));
  assert(lateEntity != NULL_ENTITY);
  const auto lateDrawables = renderSystem.collectDrawables(em, sceneGraph, 0.5f);
  const Renderer::Drawable *lateDrawable = nullptr;
  for (const auto &drawable : lateDrawables) {
    if (drawable.transform(0, 3) == 9.0f && drawable.transform(1, 3) == 0.0f &&
        drawable.transform(2, 3) == 0.0f) {
      lateDrawable = &drawable;
      break;
    }
  }
  assert(lateDrawable != nullptr);

  return 0;
}
