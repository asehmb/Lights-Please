
/*
 * This Test should be ignored for now
 * as the gravity system has been changed
 * i should prolly rewrite this
 * */
#include "../engine/entity/entity.h"
#include "../engine/entity/systems.h"
#include "../engine/job_system.h"
#include <cassert>
#include <cmath>

namespace {
bool approx(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps;
}
} // namespace

int main() {
  EntityManager em;
  SceneGraph sceneGraph(em);
  JobSystem jobSystem;
  jobSystem.initialize(1);
  GravitySystem gravitySystem;

  const ComponentMask gravityMask =
      Components::RigidBody | Components::Velocity | Components::Gravity;
  Entity_id dynamic = em.createEntity(gravityMask);
  auto *dynamicVel = static_cast<Velocity *>(
      em.getComponentData(dynamic, Components::Velocity));
  assert(dynamicVel != nullptr);
  dynamicVel->value = mathplease::Vector4(1.0f, 2.0f, 3.0f, 0.0f);

  const ComponentMask noGravityMask =
      Components::Transformable | Components::Velocity;
  Entity_id staticEntity = em.createEntity(noGravityMask);
  auto *staticTransform = static_cast<Transformable *>(
      em.getComponentData(staticEntity, Components::Transformable));
  auto *staticVel = static_cast<Velocity *>(
      em.getComponentData(staticEntity, Components::Velocity));
  assert(staticTransform != nullptr);
  assert(staticVel != nullptr);
  staticTransform->handle = sceneGraph.createTransform(staticEntity);
  sceneGraph.setLocalPosition(staticTransform->handle,
                              mathplease::Vector4(5.0f, 6.0f, 7.0f, 1.0f));
  staticVel->value = mathplease::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

  gravitySystem.update(em, &jobSystem, 1.0f);

  dynamicVel = static_cast<Velocity *>(
      em.getComponentData(dynamic, Components::Velocity));
  staticVel = static_cast<Velocity *>(
      em.getComponentData(staticEntity, Components::Velocity));
  const mathplease::Vector4 staticPos =
      sceneGraph.getLocalPosition(staticTransform->handle);

  assert(approx(dynamicVel->value.x, 1.0f));
  assert(approx(dynamicVel->value.y, -7.81f));
  assert(approx(dynamicVel->value.z, 3.0f));

  assert(approx(staticPos.x, 5.0f));
  assert(approx(staticPos.y, 6.0f));
  assert(approx(staticPos.z, 7.0f));
  assert(approx(staticVel->value.y, 0.0f));

  Collider trigger{};
  trigger.collisionLayer = CollisionLayers::Sensor;
  trigger.collisionMask = CollisionLayers::Character;
  trigger.behaviorFlags = ColliderBehavior::Trigger;

  Collider character{};
  character.collisionLayer = CollisionLayers::Character;
  character.collisionMask = CollisionLayers::Sensor;
  character.behaviorFlags = ColliderBehavior::Solid;

  assert(PhysicsSystem::passesCollisionFilter(trigger, character));
  assert(PhysicsSystem::classifyContact(trigger, character) ==
         PhysicsContactBehavior::Trigger);

  Collider staticWorld{};
  staticWorld.collisionLayer = CollisionLayers::WorldStatic;
  staticWorld.collisionMask = CollisionLayers::Default;
  staticWorld.behaviorFlags = ColliderBehavior::Solid;

  Collider dynamicBody{};
  dynamicBody.collisionLayer = CollisionLayers::Default;
  dynamicBody.collisionMask = CollisionLayers::WorldStatic;
  dynamicBody.behaviorFlags = ColliderBehavior::Solid;

  assert(PhysicsSystem::classifyContact(staticWorld, dynamicBody) ==
         PhysicsContactBehavior::Solid);

  dynamicBody.collisionMask = CollisionLayers::Sensor;
  assert(PhysicsSystem::classifyContact(staticWorld, dynamicBody) ==
         PhysicsContactBehavior::Ignore);

  return 0;
}
