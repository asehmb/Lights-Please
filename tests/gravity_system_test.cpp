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
  JobSystem jobSystem;
  jobSystem.initialize(1);
  GravitySystem gravitySystem;

  const ComponentMask gravityMask =
      Components::Position | Components::Velocity | Components::Gravity;
  Entity_id dynamic = em.createEntity(gravityMask);
  auto *dynamicPos =
      static_cast<Position *>(em.getComponentData(dynamic, Components::Position));
  auto *dynamicVel =
      static_cast<Velocity *>(em.getComponentData(dynamic, Components::Velocity));
  assert(dynamicPos != nullptr);
  assert(dynamicVel != nullptr);
  dynamicPos->value = mathplease::Vector4(0.0f, 10.0f, 0.0f, 1.0f);
  dynamicVel->value = mathplease::Vector4(1.0f, 2.0f, 3.0f, 0.0f);

  const ComponentMask noGravityMask = Components::Position | Components::Velocity;
  Entity_id staticEntity = em.createEntity(noGravityMask);
  auto *staticPos =
      static_cast<Position *>(em.getComponentData(staticEntity, Components::Position));
  auto *staticVel =
      static_cast<Velocity *>(em.getComponentData(staticEntity, Components::Velocity));
  assert(staticPos != nullptr);
  assert(staticVel != nullptr);
  staticPos->value = mathplease::Vector4(5.0f, 6.0f, 7.0f, 1.0f);
  staticVel->value = mathplease::Vector4(0.0f, 0.0f, 0.0f, 0.0f);

  gravitySystem.update(em, &jobSystem, 1.0f);

  dynamicPos =
      static_cast<Position *>(em.getComponentData(dynamic, Components::Position));
  dynamicVel =
      static_cast<Velocity *>(em.getComponentData(dynamic, Components::Velocity));
  staticPos =
      static_cast<Position *>(em.getComponentData(staticEntity, Components::Position));
  staticVel =
      static_cast<Velocity *>(em.getComponentData(staticEntity, Components::Velocity));

  assert(approx(dynamicVel->value.x, 1.0f));
  assert(approx(dynamicVel->value.y, -7.81f));
  assert(approx(dynamicVel->value.z, 3.0f));
  assert(approx(dynamicPos->value.x, 1.0f));
  assert(approx(dynamicPos->value.y, 2.19f));
  assert(approx(dynamicPos->value.z, 3.0f));

  assert(approx(staticPos->value.x, 5.0f));
  assert(approx(staticPos->value.y, 6.0f));
  assert(approx(staticPos->value.z, 7.0f));
  assert(approx(staticVel->value.y, 0.0f));

  return 0;
}
