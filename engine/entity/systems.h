

#ifndef ENTITY_SYSTEMS_H
#define ENTITY_SYSTEMS_H

#include "entity.h"
#include "../job_system.h" 

struct GravitySystem {
    void update(EntityManager& entityManager, JobSystem* jobSystem, float deltaTime);
};

#endif // ENTITY_SYSTEMS_H