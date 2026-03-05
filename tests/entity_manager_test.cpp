#include "../engine/entity/entity.h"
#include <cassert>

int main() {
  EntityManager em;

  const ComponentMask movingMask = Components::Position | Components::Velocity;
  const ComponentMask healthMask = Components::Position | Components::Health;
  Entity_id moving = em.createEntity(movingMask);
  Entity_id healthy = em.createEntity(healthMask);
  assert(moving != NULL_ENTITY);
  assert(healthy != NULL_ENTITY);

  auto *position =
      static_cast<Position *>(em.getComponentData(moving, Components::Position));
  auto *velocity =
      static_cast<Velocity *>(em.getComponentData(moving, Components::Velocity));
  assert(position != nullptr);
  assert(velocity != nullptr);
  position->value = mathplease::Vector4(1.0f, 2.0f, 3.0f, 1.0f);
  velocity->value = mathplease::Vector4(0.5f, 0.0f, -1.0f, 0.0f);

  auto *health =
      static_cast<Health *>(em.getComponentData(healthy, Components::Health));
  assert(health != nullptr);
  health->current = 10;
  health->max = 20;

  auto entitiesWithPosition = em.getAllEntitiesWithComponents(Components::Position);
  assert(entitiesWithPosition.size() >= 2);

  auto archetypes =
      em.getAllArchetypesWithComponent(Components::Position | Components::Velocity);
  assert(!archetypes.empty());

  return 0;
}
