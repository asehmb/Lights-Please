

#ifndef ENTITY_SYSTEMS_H
#define ENTITY_SYSTEMS_H

#include "../job_system.h"
#include "entity.h"
#include "sceneGraph.h"

struct GravitySystem {
  void update(EntityManager &entityManager, JobSystem *jobSystem,
              float deltaTime);
};

struct IntegrationSystem {
  void update(EntityManager &entityManager, JobSystem *jobSystem,
              float deltaTime);
};

struct SyncTransformSystem {
  void update(EntityManager &entityManager, SceneGraph &sceneGraph,
              JobSystem *jobSystem, float deltaTime);
};

enum class PhysicsContactBehavior : uint8_t { Ignore, Trigger, Solid };

struct PhysicsSystem {
  static bool passesCollisionFilter(const Collider &a, const Collider &b);
  static PhysicsContactBehavior classifyContact(const Collider &a,
                                                const Collider &b);
  void update(EntityManager &entityManager, SceneGraph &sceneGraph,
              JobSystem *jobSystem, float deltaTime);
};

#endif // ENTITY_SYSTEMS_H
