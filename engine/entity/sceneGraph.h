
#ifndef LIGHTSPLEASE_SCENEGRAPH_H
#define LIGHTSPLEASE_SCENEGRAPH_H

#include "entity.h"
#include "../job_system.h"
#include <atomic>
#include <array>
#include <vector>

#define MAX_TRANSFORMS 1048576 // 2^20

class SceneGraph {
public:
    SceneGraph(EntityManager& entityManager);
    /** Creates a transform component for the given entity */
    Transform_id createTransform(Transform_id transformId, Transform_id parentId = -1);
    /** Deletes the transform component associated with the given entity */
    void deleteTransform(Transform_id transformId);
    /** Sets the parent of a given child entity and updates the scene graph accordingly */
    void setParent(Transform_id childId, Transform_id parentId);
    /** Retrieves the parent of a given child entity */
    Transform_id getParent(Transform_id childId);

    void setLocalTRS(Transform_id transformId, const mathplease::Vector4& position,
                     const mathplease::Vector4& rotation,
                     const mathplease::Vector4& scale);

    void setLocalPosition(Transform_id transformId, const mathplease::Vector4& position);
    void setLocalRotation(Transform_id transformId, const mathplease::Vector4& rotation);
    void setLocalScale(Transform_id transformId, const mathplease::Vector4& scale);
    void markDirty(Transform_id transformId);
    
    mathplease::Vector4 getLocalPosition(Transform_id transformId);
    mathplease::Vector4 getLocalRotation(Transform_id transformId);
    mathplease::Vector4 getLocalScale(Transform_id transformId);

    /** Jobsystem may update local transform components directly without synchronization
     * e.g it might be deleted by a job while another job is reading it. but idk tho
     */
    mathplease::Vector4* getLocalPositionPtr(Transform_id transformId);
    mathplease::Vector4* getLocalRotationPtr(Transform_id transformId);
    mathplease::Vector4* getLocalScalePtr(Transform_id transformId);



    mathplease::Matrix4 getLocalMatrix(Transform_id transformId);
    mathplease::Matrix4 getWorldMatrix(Transform_id transformId);
    
    mathplease::Matrix4* getWorldMatrixPtr(Transform_id transformId);
    mathplease::Matrix4* getLocalMatrixPtr(Transform_id transformId);

    /** updates for all dirty transforms */
    void updateWorldTransforms(JobSystem* jobSystem);


private:
    EntityManager& entityManager;

    bool alive(Transform_id transformId) const;
    uint32_t inline transformIndex(Transform_id transformId) const;
    uint32_t inline transformGeneration(Transform_id transformId) const;

    void unlinkFromParent(Transform_id childId);
    void linkToParent(Transform_id childId, Transform_id parentId);

    void setDirty(Transform_id transformId);

    mathplease::Matrix4 computeLocalMatrix(Transform_id transformId);
    void computeWorldMatrixRecursive(Transform_id transformId, const mathplease::Matrix4& parentWorldMatrix);

    void updateSubtree(uint32_t topIndex);

    std::vector<mathplease::Vector4> localPositions;
    std::vector<mathplease::Vector4> localRotations;
    std::vector<mathplease::Vector4> localScales;
    std::vector<mathplease::Matrix4> localMatrices;
    std::vector<mathplease::Matrix4> worldMatrices;
    std::array<uint8_t, BITMASK_INDEX + 1> dirtyFlags; // pre-allocated for branchless access
    std::vector<uint32_t> freeIndices;
    std::vector<uint32_t> generations;
    std::vector<uint8_t> activeFlags;
    std::vector<uint32_t> dirtyIndicesScratch;
    std::vector<uint32_t> dirtyRootsScratch;
    std::atomic_bool hasDirtyTransforms{false};
    std::vector<int32_t> parentIndices; // -1 if no parent
    std::vector<int32_t> firstChildIndices; // -1 if no children
    std::vector<int32_t> nextSiblingIndices; // -1 if no next sibling
};

#endif //LIGHTSPLEASE_SCENEGRAPH_H
