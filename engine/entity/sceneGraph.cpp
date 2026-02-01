#include "sceneGraph.h"
#include "entity.h"
#include <cstdint>

#define START_CAPACITY MAX_TRANSFORMS / 12 // 2^20 / 12 ~= 87381

SceneGraph::SceneGraph(EntityManager &entityManager)
    : entityManager(entityManager) {
  localMatrices.reserve(START_CAPACITY);
  worldMatrices.reserve(START_CAPACITY);
  parentIndices.reserve(START_CAPACITY);
  firstChildIndices.reserve(START_CAPACITY);
  nextSiblingIndices.reserve(START_CAPACITY);
  dirtyFlags.fill(0);
}



void SceneGraph::setDirty(Transform_id transformId) {
    dirtyFlags[transformId & BITMASK_INDEX] = true;
}
/*
*           head
*           /
*          /
*        /  (firstChild)
*      /
* child1 (nextSiblingIndex)-> child2 (nextSiblingIndex)-> child3 (nextSiblingIndex)-> -1
*    | (firstChild)            /                             | (firstChild)
*   -1                       /                              -1
*                          /  (firstChild)
*                        /
*                    child4 -> -1
*                      | (firstChild)
*                     -1
* TODO: remove recursion. or improve it somehow
* improve: use depth to sort transforms to update for cache coherence
*/
void SceneGraph::setSubtreeDirty(Transform_id transformId) {
    uint32_t index = transformIndex(transformId);
    if (index == -1) return;
    dirtyFlags[index] = true;

    setSubtreeDirty(nextSiblingIndices[index]);
    setSubtreeDirty(firstChildIndices[index]);


}

uint32_t inline SceneGraph::transformIndex(Transform_id transformId) const {
    return transformId & BITMASK_INDEX;
}

uint32_t inline SceneGraph::transformGeneration(Transform_id transformId) const {
    return transformId & BITMASK_GENERATION;
}

Transform_id SceneGraph::getParent(Transform_id childId) {
    uint32_t index = transformIndex(childId);
    if (index >= parentIndices.size()) return -1;
    uint32_t parentIndex = parentIndices[index];
    if (parentIndex == -1) return -1;
    return (transformGeneration(childId)) | parentIndex;
}
// TODO: fix waht ai did here tmr
Transform_id SceneGraph::createTransform(Transform_id transformId, Transform_id parentId) {
    uint32_t index;
    if (transformId == NULL_ENTITY) {
        if (!freeIndices.empty()) {
            index = freeIndices.back();
            freeIndices.pop_back();
        } else {
            index = static_cast<uint32_t>(localPositions.size());
            localPositions.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
            localRotations.emplace_back(0.0f, 0.0f, 0.0f, 1.0f);
            localScales.emplace_back(1.0f, 1.0f, 1.0f, 1.0f);
            localMatrices.emplace_back(mathplease::Matrix4::identity());
            worldMatrices.emplace_back(mathplease::Matrix4::identity());
            parentIndices.push_back(-1);
            firstChildIndices.push_back(-1);
            nextSiblingIndices.push_back(-1);
            dirtyFlags[index] = true;
        }
        transformId = (0 | index); // generation 0
    } else {
        index = transformIndex(transformId);
    }

    if (parentId != NULL_ENTITY) {
        linkToParent(transformId, parentId);
    }

    setDirty(transformId);

    return transformId;
}