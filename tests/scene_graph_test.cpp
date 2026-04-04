#include "../engine/entity/sceneGraph.h"

#include <cassert>
#include <cmath>

namespace {
bool approx(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps;
}
} // namespace

int main() {
  EntityManager entityManager;
  SceneGraph sceneGraph(entityManager);
  entityManager.setEntityDestroyedCallback(
      [&](Entity_id, Transform_id transformHandle) {
        if (transformHandle != NULL_ENTITY) {
          sceneGraph.deleteTransform(transformHandle);
        }
      });

  Transform_id root = sceneGraph.createTransform(1);
  Transform_id child = sceneGraph.createTransform(2, root);

  assert(root != NULL_ENTITY);
  assert(child != NULL_ENTITY);
  assert(sceneGraph.getParent(child) == root);

  sceneGraph.setLocalPosition(root, mathplease::Vector4(1.0f, 2.0f, 3.0f, 1.0f));
  sceneGraph.setLocalPosition(child, mathplease::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
  sceneGraph.updateWorldTransforms(nullptr);

  mathplease::Matrix4 childWorld = sceneGraph.getWorldMatrix(child);
  assert(approx(childWorld(0, 3), 1.0f));
  assert(approx(childWorld(1, 3), 3.0f));
  assert(approx(childWorld(2, 3), 3.0f));

  sceneGraph.setParent(child, NULL_ENTITY);
  sceneGraph.updateWorldTransforms(nullptr);
  assert(sceneGraph.getParent(child) == NULL_ENTITY);
  childWorld = sceneGraph.getWorldMatrix(child);
  assert(approx(childWorld(0, 3), 0.0f));
  assert(approx(childWorld(1, 3), 1.0f));
  assert(approx(childWorld(2, 3), 0.0f));

  Entity_id firstEntity = entityManager.createEntity(Components::Transformable);
  auto *firstTransformable = static_cast<Transformable *>(
      entityManager.getComponentData(firstEntity, Components::Transformable));
  assert(firstTransformable != nullptr);
  firstTransformable->handle = sceneGraph.createTransform(firstEntity);
  sceneGraph.setLocalPosition(firstTransformable->handle,
                              mathplease::Vector4(7.0f, 8.0f, 9.0f, 1.0f));
  entityManager.destroyEntity(firstEntity);

  Entity_id secondEntity = entityManager.createEntity(Components::Transformable);
  auto *secondTransformable = static_cast<Transformable *>(
      entityManager.getComponentData(secondEntity, Components::Transformable));
  assert(secondTransformable != nullptr);
  secondTransformable->handle = sceneGraph.createTransform(secondEntity);
  const mathplease::Vector4 defaultPosition =
      sceneGraph.getLocalPosition(secondTransformable->handle);
  assert(secondEntity != firstEntity);
  assert(approx(defaultPosition.x, 0.0f));
  assert(approx(defaultPosition.y, 0.0f));
  assert(approx(defaultPosition.z, 0.0f));

  return 0;
}
