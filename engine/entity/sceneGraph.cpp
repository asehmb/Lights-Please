#include "sceneGraph.h"

#include <algorithm>
#include <cstdint>

#define START_CAPACITY MAX_TRANSFORMS / 12 // 2^20 / 12 ~= 87381

namespace {
constexpr uint32_t kGenerationStride = BITMASK_INDEX + 1;

Transform_id composeTransformId(uint32_t generation, uint32_t index) {
  return (generation & BITMASK_GENERATION) | (index & BITMASK_INDEX);
}
} // namespace

SceneGraph::SceneGraph(EntityManager &entityManager) : entityManager(entityManager) {
  localPositions.reserve(START_CAPACITY);
  localRotations.reserve(START_CAPACITY);
  localScales.reserve(START_CAPACITY);
  localMatrices.reserve(START_CAPACITY);
  worldMatrices.reserve(START_CAPACITY);
  parentIndices.reserve(START_CAPACITY);
  firstChildIndices.reserve(START_CAPACITY);
  nextSiblingIndices.reserve(START_CAPACITY);
  generations.reserve(START_CAPACITY);
  activeFlags.reserve(START_CAPACITY);
  dirtyFlags.fill(0);
}

bool SceneGraph::alive(Transform_id transformId) const {
  const uint32_t index = transformIndex(transformId);
  if (index >= activeFlags.size()) {
    return false;
  }
  if (!activeFlags[index]) {
    return false;
  }
  return generations[index] == transformGeneration(transformId);
}

uint32_t inline SceneGraph::transformIndex(Transform_id transformId) const {
  return transformId & BITMASK_INDEX;
}

uint32_t inline SceneGraph::transformGeneration(Transform_id transformId) const {
  return transformId & BITMASK_GENERATION;
}

void SceneGraph::setDirty(Transform_id transformId) {
  const uint32_t index = transformIndex(transformId);
  if (index >= dirtyFlags.size()) {
    return;
  }
  dirtyFlags[index] = 1;
  hasDirtyTransforms.store(true, std::memory_order_relaxed);
}

void SceneGraph::unlinkFromParent(Transform_id childId) {
  if (!alive(childId)) {
    return;
  }

  const uint32_t childIndex = transformIndex(childId);
  const int32_t parentIndex = parentIndices[childIndex];
  if (parentIndex == -1) {
    return;
  }

  uint32_t parent = static_cast<uint32_t>(parentIndex);
  if (firstChildIndices[parent] == static_cast<int32_t>(childIndex)) {
    firstChildIndices[parent] = nextSiblingIndices[childIndex];
  } else {
    int32_t sibling = firstChildIndices[parent];
    while (sibling != -1) {
      uint32_t siblingIndex = static_cast<uint32_t>(sibling);
      if (nextSiblingIndices[siblingIndex] == static_cast<int32_t>(childIndex)) {
        nextSiblingIndices[siblingIndex] = nextSiblingIndices[childIndex];
        break;
      }
      sibling = nextSiblingIndices[siblingIndex];
    }
  }

  parentIndices[childIndex] = -1;
  nextSiblingIndices[childIndex] = -1;
  setDirty(childId);
}

void SceneGraph::linkToParent(Transform_id childId, Transform_id parentId) {
  if (!alive(childId) || !alive(parentId) || childId == parentId) {
    return;
  }

  const uint32_t childIndex = transformIndex(childId);
  const uint32_t parentIndex = transformIndex(parentId);

  int32_t ancestor = static_cast<int32_t>(parentIndex);
  while (ancestor != -1) {
    if (static_cast<uint32_t>(ancestor) == childIndex) {
      return;
    }
    ancestor = parentIndices[static_cast<uint32_t>(ancestor)];
  }

  unlinkFromParent(childId);
  nextSiblingIndices[childIndex] = firstChildIndices[parentIndex];
  firstChildIndices[parentIndex] = static_cast<int32_t>(childIndex);
  parentIndices[childIndex] = static_cast<int32_t>(parentIndex);
  setDirty(childId);
}

Transform_id SceneGraph::createTransform(Transform_id transformId, Transform_id parentId) {
  uint32_t index = 0;

  if (transformId == NULL_ENTITY) {
    if (!freeIndices.empty()) {
      index = freeIndices.back();
      freeIndices.pop_back();
      activeFlags[index] = 1;
      localPositions[index] = mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
      localRotations[index] = mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
      localScales[index] = mathplease::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      localMatrices[index] = mathplease::Matrix4::identity();
      worldMatrices[index] = mathplease::Matrix4::identity();
      parentIndices[index] = -1;
      firstChildIndices[index] = -1;
      nextSiblingIndices[index] = -1;
    } else {
      index = static_cast<uint32_t>(localPositions.size());
      if (index >= MAX_TRANSFORMS) {
        return NULL_ENTITY;
      }
      localPositions.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
      localRotations.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
      localScales.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
      localMatrices.emplace_back(mathplease::Matrix4::identity());
      worldMatrices.emplace_back(mathplease::Matrix4::identity());
      parentIndices.push_back(-1);
      firstChildIndices.push_back(-1);
      nextSiblingIndices.push_back(-1);
      generations.push_back(0);
      activeFlags.push_back(1);
    }
    transformId = composeTransformId(generations[index], index);
  } else {
    index = transformIndex(transformId);
    if (index >= MAX_TRANSFORMS) {
      return NULL_ENTITY;
    }

    if (index >= localPositions.size()) {
      localPositions.resize(index + 1, mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
      localRotations.resize(index + 1, mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
      localScales.resize(index + 1, mathplease::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
      localMatrices.resize(index + 1, mathplease::Matrix4::identity());
      worldMatrices.resize(index + 1, mathplease::Matrix4::identity());
      parentIndices.resize(index + 1, -1);
      firstChildIndices.resize(index + 1, -1);
      nextSiblingIndices.resize(index + 1, -1);
      generations.resize(index + 1, 0);
      activeFlags.resize(index + 1, 0);
    }

    if (activeFlags[index]) {
      if (generations[index] != transformGeneration(transformId)) {
        return NULL_ENTITY;
      }
    } else {
      generations[index] = transformGeneration(transformId);
      activeFlags[index] = 1;
      localPositions[index] = mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
      localRotations[index] = mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
      localScales[index] = mathplease::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
      localMatrices[index] = mathplease::Matrix4::identity();
      worldMatrices[index] = mathplease::Matrix4::identity();
      parentIndices[index] = -1;
      firstChildIndices[index] = -1;
      nextSiblingIndices[index] = -1;
      freeIndices.erase(std::remove(freeIndices.begin(), freeIndices.end(), index),
                        freeIndices.end());
    }
    transformId = composeTransformId(generations[index], index);
  }

  if (parentId != NULL_ENTITY) {
    setParent(transformId, parentId);
  }
  setDirty(transformId);
  return transformId;
}

void SceneGraph::deleteTransform(Transform_id transformId) {
  if (!alive(transformId)) {
    return;
  }

  const uint32_t index = transformIndex(transformId);
  unlinkFromParent(transformId);

  int32_t child = firstChildIndices[index];
  while (child != -1) {
    const uint32_t childIndex = static_cast<uint32_t>(child);
    const int32_t next = nextSiblingIndices[childIndex];
    parentIndices[childIndex] = -1;
    nextSiblingIndices[childIndex] = -1;
    dirtyFlags[childIndex] = 1;
    hasDirtyTransforms.store(true, std::memory_order_relaxed);
    child = next;
  }
  firstChildIndices[index] = -1;

  activeFlags[index] = 0;
  dirtyFlags[index] = 0;
  generations[index] = (generations[index] + kGenerationStride) & BITMASK_GENERATION;
  hasDirtyTransforms.store(true, std::memory_order_relaxed);
  freeIndices.push_back(index);
}

void SceneGraph::setParent(Transform_id childId, Transform_id parentId) {
  if (!alive(childId)) {
    return;
  }
  if (parentId == NULL_ENTITY) {
    unlinkFromParent(childId);
    return;
  }
  if (!alive(parentId)) {
    return;
  }

  const uint32_t childIndex = transformIndex(childId);
  if (parentIndices[childIndex] == static_cast<int32_t>(transformIndex(parentId))) {
    return;
  }
  linkToParent(childId, parentId);
}

Transform_id SceneGraph::getParent(Transform_id childId) {
  if (!alive(childId)) {
    return NULL_ENTITY;
  }
  const uint32_t index = transformIndex(childId);
  const int32_t parentIndex = parentIndices[index];
  if (parentIndex == -1) {
    return NULL_ENTITY;
  }
  const uint32_t resolvedParentIndex = static_cast<uint32_t>(parentIndex);
  return composeTransformId(generations[resolvedParentIndex], resolvedParentIndex);
}

void SceneGraph::setLocalTRS(Transform_id transformId,
                             const mathplease::Vector4 &position,
                             const mathplease::Vector4 &rotation,
                             const mathplease::Vector4 &scale) {
  if (!alive(transformId)) {
    return;
  }
  const uint32_t index = transformIndex(transformId);
  localPositions[index] = position;
  localRotations[index] = rotation;
  localScales[index] = scale;
  setDirty(transformId);
}

void SceneGraph::setLocalPosition(Transform_id transformId,
                                  const mathplease::Vector4 &position) {
  if (!alive(transformId)) {
    return;
  }
  localPositions[transformIndex(transformId)] = position;
  setDirty(transformId);
}

void SceneGraph::setLocalRotation(Transform_id transformId,
                                  const mathplease::Vector4 &rotation) {
  if (!alive(transformId)) {
    return;
  }
  localRotations[transformIndex(transformId)] = rotation;
  setDirty(transformId);
}

void SceneGraph::setLocalScale(Transform_id transformId,
                               const mathplease::Vector4 &scale) {
  if (!alive(transformId)) {
    return;
  }
  localScales[transformIndex(transformId)] = scale;
  setDirty(transformId);
}

void SceneGraph::markDirty(Transform_id transformId) { setDirty(transformId); }

mathplease::Vector4 SceneGraph::getLocalPosition(Transform_id transformId) {
  if (!alive(transformId)) {
    return mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  }
  return localPositions[transformIndex(transformId)];
}

mathplease::Vector4 SceneGraph::getLocalRotation(Transform_id transformId) {
  if (!alive(transformId)) {
    return mathplease::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
  }
  return localRotations[transformIndex(transformId)];
}

mathplease::Vector4 SceneGraph::getLocalScale(Transform_id transformId) {
  if (!alive(transformId)) {
    return mathplease::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
  }
  return localScales[transformIndex(transformId)];
}

mathplease::Vector4 *SceneGraph::getLocalPositionPtr(Transform_id transformId) {
  if (!alive(transformId)) {
    return nullptr;
  }
  return &localPositions[transformIndex(transformId)];
}

mathplease::Vector4 *SceneGraph::getLocalRotationPtr(Transform_id transformId) {
  if (!alive(transformId)) {
    return nullptr;
  }
  return &localRotations[transformIndex(transformId)];
}

mathplease::Vector4 *SceneGraph::getLocalScalePtr(Transform_id transformId) {
  if (!alive(transformId)) {
    return nullptr;
  }
  return &localScales[transformIndex(transformId)];
}

mathplease::Matrix4 SceneGraph::computeLocalMatrix(Transform_id transformId) {
  if (!alive(transformId)) {
    return mathplease::Matrix4::identity();
  }

  const uint32_t index = transformIndex(transformId);
  const mathplease::Vector4 &position = localPositions[index];
  const mathplease::Vector4 &rotation = localRotations[index];
  const mathplease::Vector4 &scale = localScales[index];

  const mathplease::Matrix4 translation = mathplease::Matrix4::translate(position.xyz());
  const mathplease::Matrix4 rotationX = mathplease::Matrix4::rotateX(rotation.x);
  const mathplease::Matrix4 rotationY = mathplease::Matrix4::rotateY(rotation.y);
  const mathplease::Matrix4 rotationZ = mathplease::Matrix4::rotateZ(rotation.z);
  const mathplease::Matrix4 scaling = mathplease::Matrix4::scale(scale.xyz());
  return translation * rotationZ * rotationY * rotationX * scaling;
}

void SceneGraph::computeWorldMatrixRecursive(
    Transform_id transformId, const mathplease::Matrix4 &parentWorldMatrix) {
  if (!alive(transformId)) {
    return;
  }

  const uint32_t index = transformIndex(transformId);
  localMatrices[index] = computeLocalMatrix(transformId);
  worldMatrices[index] = parentWorldMatrix * localMatrices[index];
  dirtyFlags[index] = 0;

  int32_t child = firstChildIndices[index];
  while (child != -1) {
    const uint32_t childIndex = static_cast<uint32_t>(child);
    const Transform_id childId = composeTransformId(generations[childIndex], childIndex);
    computeWorldMatrixRecursive(childId, worldMatrices[index]);
    child = nextSiblingIndices[childIndex];
  }
}

void SceneGraph::updateSubtree(uint32_t topIndex) {
  if (topIndex >= activeFlags.size() || !activeFlags[topIndex]) {
    return;
  }

  const int32_t parentIndex = parentIndices[topIndex];
  const Transform_id topId = composeTransformId(generations[topIndex], topIndex);
  if (parentIndex == -1) {
    computeWorldMatrixRecursive(topId, mathplease::Matrix4::identity());
    return;
  }

  const uint32_t resolvedParent = static_cast<uint32_t>(parentIndex);
  if (dirtyFlags[resolvedParent]) {
    updateSubtree(resolvedParent);
  }
  computeWorldMatrixRecursive(topId, worldMatrices[resolvedParent]);
}

void SceneGraph::updateWorldTransforms(JobSystem *jobSystem) {
  if (!hasDirtyTransforms.load(std::memory_order_relaxed)) {
    return;
  }

  dirtyIndicesScratch.clear();
  dirtyIndicesScratch.reserve(activeFlags.size());
  dirtyRootsScratch.clear();
  dirtyRootsScratch.reserve(dirtyRootsScratch.capacity());

  for (uint32_t index = 0; index < activeFlags.size(); ++index) {
    if (!activeFlags[index] || !dirtyFlags[index]) {
      continue;
    }
    dirtyIndicesScratch.push_back(index);

    const int32_t parent = parentIndices[index];
    if (parent != -1 && dirtyFlags[static_cast<uint32_t>(parent)]) {
      continue;
    }
    dirtyRootsScratch.push_back(index);
  }

  if (dirtyRootsScratch.empty()) {
    for (uint32_t index : dirtyIndicesScratch) {
      dirtyFlags[index] = 0;
    }
    hasDirtyTransforms.store(false, std::memory_order_relaxed);
    return;
  }

  if (!jobSystem || dirtyRootsScratch.size() <= 1) {
    for (uint32_t index : dirtyRootsScratch) {
      updateSubtree(index);
    }
    for (uint32_t index : dirtyIndicesScratch) {
      dirtyFlags[index] = 0;
    }
    hasDirtyTransforms.store(false, std::memory_order_relaxed);
    return;
  }

  JobCounter counter{};
  for (uint32_t index : dirtyRootsScratch) {
    jobSystem->kickJob([this, index]() { updateSubtree(index); }, &counter);
  }
  jobSystem->waitForCounter(&counter);
  for (uint32_t index : dirtyIndicesScratch) {
    dirtyFlags[index] = 0;
  }
  hasDirtyTransforms.store(false, std::memory_order_relaxed);
}

mathplease::Matrix4 SceneGraph::getLocalMatrix(Transform_id transformId) {
  if (!alive(transformId)) {
    return mathplease::Matrix4::identity();
  }
  const uint32_t index = transformIndex(transformId);
  if (dirtyFlags[index]) {
    localMatrices[index] = computeLocalMatrix(transformId);
  }
  return localMatrices[index];
}

mathplease::Matrix4 SceneGraph::getWorldMatrix(Transform_id transformId) {
  if (!alive(transformId)) {
    return mathplease::Matrix4::identity();
  }
  const uint32_t index = transformIndex(transformId);
  if (dirtyFlags[index]) {
    updateSubtree(index);
  }
  return worldMatrices[index];
}

mathplease::Matrix4 *SceneGraph::getWorldMatrixPtr(Transform_id transformId) {
  if (!alive(transformId)) {
    return nullptr;
  }
  const uint32_t index = transformIndex(transformId);
  if (dirtyFlags[index]) {
    updateSubtree(index);
  }
  return &worldMatrices[index];
}

mathplease::Matrix4 *SceneGraph::getLocalMatrixPtr(Transform_id transformId) {
  if (!alive(transformId)) {
    return nullptr;
  }
  const uint32_t index = transformIndex(transformId);
  if (dirtyFlags[index]) {
    localMatrices[index] = computeLocalMatrix(transformId);
  }
  return &localMatrices[index];
}
