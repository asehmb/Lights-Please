
#include "systems.h"
#include "../math/vector.hpp"
#include "entity.h"

constexpr float GRAVITY_ACCELERATION = 9.81f;

bool PhysicsSystem::passesCollisionFilter(const Collider &a,
                                          const Collider &b) {
  return a.layerMaskPasses(b);
}

PhysicsContactBehavior PhysicsSystem::classifyContact(const Collider &a,
                                                      const Collider &b) {
  if (!passesCollisionFilter(a, b)) {
    return PhysicsContactBehavior::Ignore;
  }

  if (a.isTrigger() || b.isTrigger()) {
    return PhysicsContactBehavior::Trigger;
  }

  if (a.isSolid() && b.isSolid()) {
    return PhysicsContactBehavior::Solid;
  }

  return PhysicsContactBehavior::Ignore;
}

void PhysicsSystem::update(EntityManager &entityManager, SceneGraph &sceneGraph,
                           JobSystem *jobSystem, float deltaTime) {
  (void)entityManager;
  (void)sceneGraph;
  (void)jobSystem;
  (void)deltaTime;
}

// Only applies gravity to dynamic rigid bodies, static bodies are unaffected
void GravitySystem::update(EntityManager &entityManager, JobSystem *jobSystem,
                           float deltaTime) {
  static ComponentMask requiredComponents =
      Components::Gravity | Components::Velocity | Components::RigidBody;

  const auto &archetypes =
      entityManager.getAllArchetypesWithComponent(requiredComponents);
  if (archetypes.empty())
    return;

  JobCounter counter = {};
  for (Archetype *archetype : archetypes) {
    uint32_t velOffset =
        archetype->offsets[componentMaskToIndex(Components::Velocity)];
    uint32_t rbOffset =
        archetype->offsets[componentMaskToIndex(Components::RigidBody)];

    for (Chunk *chunk : archetype->chunks) {
      char *chunkData = (char *)chunk->data;
      Velocity *velocities = (Velocity *)(chunkData + velOffset);
      RigidBody *rigidBodies = (RigidBody *)(chunkData + rbOffset);
      uint32_t entityCount = chunk->row;

      auto job = [velocities, rigidBodies, deltaTime, entityCount]() {
        for (uint32_t i = 0; i < entityCount; ++i) {
          if (rigidBodies[i].type != RigidBodyType::Dynamic) {
            continue;
          }

          velocities[i].value.y -= GRAVITY_ACCELERATION * deltaTime;
        }
      };

      jobSystem->kickJob(job, &counter);
    }
  }

  jobSystem->waitForCounter(&counter);
}

void IntegrationSystem::update(EntityManager &entityManager,
                               JobSystem *jobSystem, float deltaTime) {
  static ComponentMask requiredComponents =
      Components::Position | Components::Velocity | Components::RigidBody;

  const auto &archetypes =
      entityManager.getAllArchetypesWithComponent(requiredComponents);
  if (archetypes.empty())
    return;

  JobCounter counter = {};
  for (Archetype *archetype : archetypes) {
    uint32_t posOffset =
        archetype->offsets[componentMaskToIndex(Components::Position)];
    uint32_t velOffset =
        archetype->offsets[componentMaskToIndex(Components::Velocity)];
    uint32_t rbOffset =
        archetype->offsets[componentMaskToIndex(Components::RigidBody)];

    for (Chunk *chunk : archetype->chunks) {
      char *chunkData = (char *)chunk->data;
      Position *positions = (Position *)(chunkData + posOffset);
      Velocity *velocities = (Velocity *)(chunkData + velOffset);
      RigidBody *rigidBodies = (RigidBody *)(chunkData + rbOffset);
      uint32_t entityCount = chunk->row;

      auto job = [positions, velocities, rigidBodies, deltaTime,
                  entityCount]() {
        for (uint32_t i = 0; i < entityCount; ++i) {
          if (rigidBodies[i].type == RigidBodyType::Static) {
            continue;
          }

          positions[i].value.x += velocities[i].value.x * deltaTime;
          positions[i].value.y += velocities[i].value.y * deltaTime;
          positions[i].value.z += velocities[i].value.z * deltaTime;
        }
      };

      jobSystem->kickJob(job, &counter);
    }
  }

  jobSystem->waitForCounter(&counter);
}

void SyncTransformSystem::update(EntityManager &entityManager,
                                 SceneGraph &sceneGraph, JobSystem *jobSystem,
                                 float deltaTime) {
  static ComponentMask requiredComponents =
      Components::Position | Components::Transformable;

  const auto &archetypes =
      entityManager.getAllArchetypesWithComponent(requiredComponents);
  if (archetypes.empty())
    return;

  JobCounter counter = {};
  for (Archetype *archetype : archetypes) {
    uint32_t posOffset =
        archetype->offsets[componentMaskToIndex(Components::Position)];
    uint32_t transformOffset =
        archetype->offsets[componentMaskToIndex(Components::Transformable)];

    for (Chunk *chunk : archetype->chunks) {
      char *chunkData = (char *)chunk->data;
      Position *positions = (Position *)(chunkData + posOffset);
      Transformable *transformables =
          (Transformable *)(chunkData + transformOffset);
      uint32_t entityCount = chunk->row;

      auto job = [positions, transformables, entityCount, &sceneGraph]() {
        for (uint32_t i = 0; i < entityCount; ++i) {
          Transform_id handle = transformables[i].handle;
          mathplease::Vector4 *pos = sceneGraph.getLocalPositionPtr(handle);
          if (!pos)
            continue;

          *pos = positions[i].value;
          sceneGraph.markDirty(handle);
        }
      };

      jobSystem->kickJob(job, &counter);
    }
  }

  jobSystem->waitForCounter(&counter);
}
