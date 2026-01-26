
#include "systems.h"
#include "../math/vector.hpp"



constexpr float GRAVITY_ACCELERATION = 9.81f;

void GravitySystem::update(EntityManager& entityManager, JobSystem* jobSystem, float deltaTime) {
    ComponentMask requiredComponents = Components::Position | Components::Velocity | Components::Gravity;

    std::vector<Archetype*> archetypes = 
        entityManager.getAllArchetypesWithComponent(requiredComponents);
    if (archetypes.empty()) return;

    JobCounter counter = {};
    for (Archetype* archetype : archetypes) {
        uint32_t posOffset = archetype->offsets[Components::Position];
        uint32_t velOffset = archetype->offsets[Components::Velocity];

        // Iterate through all chunks in the archetype
        for (Chunk* chunk : archetype->chunks) {
            char* chunkData = (char*)chunk->data;
            char* posBase = chunkData + posOffset;
            char* velBase = chunkData + velOffset;
            
            // Component arrays
            Position* positions = (Position*)posBase;
            Velocity* velocities = (Velocity*)velBase;
            uint32_t entityCount = chunk->row;
            
            // Capture logic for the job
            auto job = [positions, velocities, deltaTime, entityCount]() {
                
                for (uint32_t i = 0; i < entityCount; ++i) {
                    mathplease::Vector4& pos = positions[i].value;
                    mathplease::Vector4& vel = velocities[i].value;
                    
                    vel.y -= GRAVITY_ACCELERATION * deltaTime;
                    
                    pos.x += vel.x * deltaTime;
                    pos.y += vel.y * deltaTime;
                    pos.z += vel.z * deltaTime;
                }
            };
            
            // Kick the job
            jobSystem->kickJob(job, &counter);
        }
    }
    
    // Wait for all chunks to be processed
    jobSystem->waitForCounter(&counter);
}