#include "../engine/entity/entity.h"
#include <cassert>

int main() {
  EntityManager em;

  const ComponentMask movingMask = Components::Transformable | Components::Velocity;
  const ComponentMask healthMask = Components::Transformable | Components::Health;
  Entity_id moving = em.createEntity(movingMask);
  Entity_id healthy = em.createEntity(healthMask);
  assert(moving != NULL_ENTITY);
  assert(healthy != NULL_ENTITY);

  auto *transformable = static_cast<Transformable *>(
      em.getComponentData(moving, Components::Transformable));
  auto *velocity =
      static_cast<Velocity *>(em.getComponentData(moving, Components::Velocity));
  assert(transformable != nullptr);
  assert(velocity != nullptr);
  transformable->handle = moving;
  velocity->value = mathplease::Vector4(0.5f, 0.0f, -1.0f, 0.0f);

  auto *health =
      static_cast<Health *>(em.getComponentData(healthy, Components::Health));
  assert(health != nullptr);
  health->current = 10;
  health->max = 20;

  Entity_id colliderEntity = em.createEntity(Components::Collider);
  assert(colliderEntity != NULL_ENTITY);
  auto *collider = static_cast<Collider *>(
      em.getComponentData(colliderEntity, Components::Collider));
  assert(collider != nullptr);
  assert(collider->collisionLayer == CollisionLayers::Default);
  assert(collider->collisionMask == CollisionLayers::All);
  assert(collider->isSolid());
  assert(!collider->isTrigger());

  auto entitiesWithTransform =
      em.getAllEntitiesWithComponents(Components::Transformable);
  assert(entitiesWithTransform.size() >= 2);

  auto archetypes =
      em.getAllArchetypesWithComponent(Components::Transformable |
                                       Components::Velocity);
  assert(!archetypes.empty());

  Entity_id recycled = em.createEntity(Components::Transformable);
  auto *recycledTransformable = static_cast<Transformable *>(
      em.getComponentData(recycled, Components::Transformable));
  assert(recycledTransformable != nullptr);
  recycledTransformable->handle = 1337;

  bool destroyCallbackInvoked = false;
  Entity_id destroyedEntity = NULL_ENTITY;
  Transform_id destroyedHandle = NULL_ENTITY;
  em.setEntityDestroyedCallback(
      [&](Entity_id entityId, Transform_id transformHandle) {
        destroyCallbackInvoked = true;
        destroyedEntity = entityId;
        destroyedHandle = transformHandle;
      });

  em.destroyEntity(recycled);
  assert(destroyCallbackInvoked);
  assert(destroyedEntity == recycled);
  assert(destroyedHandle == 1337);
  assert(em.getComponentData(recycled, Components::Transformable) == nullptr);

  Entity_id recycledNext = em.createEntity(Components::Transformable);
  assert(recycledNext != recycled);
  assert(em.getComponentData(recycled, Components::Transformable) == nullptr);

  Entity_id triggerEntity = em.createEntity(Components::Collider);
  auto *triggerCollider = static_cast<Collider *>(
      em.getComponentData(triggerEntity, Components::Collider));
  assert(triggerCollider != nullptr);
  triggerCollider->collisionLayer = CollisionLayers::Sensor;
  triggerCollider->collisionMask = CollisionLayers::Character;
  triggerCollider->behaviorFlags = ColliderBehavior::Trigger;

  Entity_id characterEntity = em.createEntity(Components::Collider);
  auto *characterCollider = static_cast<Collider *>(
      em.getComponentData(characterEntity, Components::Collider));
  assert(characterCollider != nullptr);
  characterCollider->collisionLayer = CollisionLayers::Character;
  characterCollider->collisionMask = CollisionLayers::Sensor;
  characterCollider->behaviorFlags = ColliderBehavior::Solid;

  assert(triggerCollider->layerMaskPasses(*characterCollider));
  assert(!triggerCollider->isSolid());
  assert(triggerCollider->isTrigger());

  return 0;
}
