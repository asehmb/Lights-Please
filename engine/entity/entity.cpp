
#include "entity.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>


uint32_t alignUp(uint32_t offset, size_t alignment) {
    return (offset + (alignment - 1)) & ~(alignment - 1);
}

EntityManager::EntityManager() {
    entityCount = 0;
    entityCapacity = 4194304; // preallocate for 4 million entities 2^22
    nextEntityId = 1; // Start IDs from 1
    entityRecords.reserve(4194304);
}

EntityManager::~EntityManager() {
    for (auto& arch : existingArchetypes) {
        for (auto& chunk : arch->chunks) {
            chunkAllocator.deallocate(chunk->data);
            delete chunk;
        }
    }
}

/*
 * Creates a new entity with the specified component mask.
 * Allocates space in an appropriate chunk and updates records.
 */
Entity_id EntityManager::createEntity(ComponentMask components) {
    ensureEntityCapacity();

    Archetype* archetype = getOrCreateArchetype(components); // check if exists or create new
    Chunk* chunk = getOrCreateChunk(archetype); // get or create chunk with space
    if (!chunk) {
        // Handle allocation failure
        return NULL_ENTITY;
    }

    std::uint32_t row = chunk->row; // current row in chunk

    // Store entity location
    EntityData entityData;
    entityData.archetype = archetype;
    entityData.chunk = chunk;
    entityData.row = row;

    chunk->row++; // advance row for next entity

    Entity_id finalId;
    if (freeEntityIds.empty()) {
        entityData.row = row;
        entityRecords.push_back(entityData);
        entityCount++;
        finalId = nextEntityId++;
    } else {
        uint32_t reusedId = freeEntityIds.back();
        freeEntityIds.pop_back();
        entityRecords[(reusedId & ENTITY_INDEX)] = entityData;
        finalId = reusedId & ENTITY_INDEX | // get index
            (((reusedId & ENTITY_GENERATION) + 1) // increment generation
             & ENTITY_GENERATION); // wrap around generation
    }

    // record id in chunk
    Entity_id* ids = (Entity_id*)chunk->data;
    ids[row] = finalId;


    return finalId;

}

/*
 * Ensures that the entity records array has enough capacity
 * to store new entities, resizing if necessary.
 */
void EntityManager::ensureEntityCapacity() {
    if (entityCount >= entityCapacity) {
        entityCapacity = (entityCapacity == 0) ? 16 : entityCapacity * 2;
        entityRecords.resize(entityCapacity);
    }
}

/*
 * Retrieves an existing archetype matching the component mask,
 * or creates a new one if none exists.
 */
Archetype* EntityManager::getOrCreateArchetype(ComponentMask mask) {
    for (auto& archPtr : existingArchetypes) {
        if (archPtr->componentMask == mask) return archPtr.get();
    }

    auto newArch = std::make_unique<Archetype>();
    newArch->componentMask = mask;

    // The ID array will always be at offset 0.
    size_t bytesPerEntity = sizeof(Entity_id); 
    
    for (int i = 0; i < 16; ++i) {
        if ((mask >> i) & 1) {
            ComponentInfo info = ComponentRegistry::getInfo(i);
            newArch->sizes[i] = info.size;
            bytesPerEntity += info.size;
        } else {
            newArch->sizes[i] = 0;
        }
    }

    newArch->rowSize = bytesPerEntity;
    newArch->chunkCapacity = CHUNK_SIZE / bytesPerEntity;

    // Calculate offsets
    // Start offsets AFTER the EntityID array
    // (Size of ID array = sizeof(Entity_id) * capacity)
    uint32_t currentOffset = sizeof(Entity_id) * newArch->chunkCapacity;

    for (int i = 0; i < 16; ++i) {
        if ((mask >> i) & 1) {
            // Optional: You should strictly use alignUp(currentOffset, 16) here for SIMD
            newArch->offsets[i] = currentOffset;
            currentOffset += newArch->sizes[i] * newArch->chunkCapacity;
        } else {
            newArch->offsets[i] = 0;
        }
    }

    existingArchetypes.push_back(std::move(newArch));
    return existingArchetypes.back().get();
}

/*
 * Returns a pointer to a chunk with available space for the given archetype.
 * If no such chunk exists, a new one is allocated.
 */
Chunk* EntityManager::getOrCreateChunk(Archetype* archetype) {
    // 1. Check existing last chunk
    if (!archetype->chunks.empty()) {
        Chunk* last = archetype->chunks.back();
        if (last->row < last->capacity) {
            return last;
        }
    }

    // 2. Allocate
    void* chunkData = chunkAllocator.allocate();
    if (!chunkData) return nullptr;

    Chunk* newChunk = new Chunk(); 
    newChunk->data = chunkData;
    newChunk->row = 0;
    newChunk->capacity = archetype->chunkCapacity;
    newChunk->archetype = archetype;

    archetype->chunks.push_back(newChunk);

    return newChunk;
}

// Helper: Moves a single entity from one chunk to another
void EntityManager::moveEntity(Chunk* srcChunk, uint32_t srcRow, Chunk* dstChunk) {
    Archetype* arch = srcChunk->archetype;
    uint32_t dstRow = dstChunk->row;

    // 1. Copy Components
    for (int i = 0; i < 16; ++i) {
        if (arch->sizes[i] == 0) continue;
        size_t size = arch->sizes[i];
        size_t offset = arch->offsets[i];

        std::byte* srcPtr = (std::byte*)srcChunk->data + offset + (srcRow * size);
        std::byte* dstPtr = (std::byte*)dstChunk->data + offset + (dstRow * size);
        std::memcpy(dstPtr, srcPtr, size);
    }

    // 2. Copy ID
    Entity_id* srcIds = (Entity_id*)srcChunk->data;
    Entity_id* dstIds = (Entity_id*)dstChunk->data;
    Entity_id id = srcIds[srcRow];
    dstIds[dstRow] = id;

    // 3. Update Global Record
    entityRecords[id & ENTITY_INDEX].chunk = dstChunk;
    entityRecords[id & ENTITY_INDEX].row = dstRow;

    // 4. Update Destination Count
    dstChunk->row++;
}

// Logic to check previous chunks for space
void EntityManager::tryMergeAndFreeChunk(Chunk* chunk) {
    Archetype* arch = chunk->archetype;

    // Case 1: Empty -> Just Delete
    if (chunk->row == 0) {
        // Remove from vector
        auto it = std::find(arch->chunks.begin(), arch->chunks.end(), chunk);
        if (it != arch->chunks.end()) {
            *it = arch->chunks.back();
            arch->chunks.pop_back();
        }
        chunkAllocator.deallocate(chunk->data);
        delete chunk;
        return;
    }

    if (chunk->row == 1) {
        auto it = std::find(arch->chunks.begin(), arch->chunks.end(), chunk);
        if (it != arch->chunks.begin()) {
            Chunk* prevChunk = *std::prev(it);
            
            if (prevChunk->row < prevChunk->capacity) {
                moveEntity(chunk, 0, prevChunk);
                
                chunk->row = 0;
                tryMergeAndFreeChunk(chunk); 
            }
        }
    }
}

// Returns the ID of the entity that was moved to fill the hole
Entity_id EntityManager::swapAndPopChunkRow(uint16_t rowToDelete, Chunk* chunk) {
    uint32_t lastRowIndex = chunk->row - 1; 
    Entity_id movedId = NULL_ENTITY;

    if (rowToDelete != lastRowIndex) {
        Archetype* arch = chunk->archetype;
        
        // A. Move Components
        for (int i = 0; i < 16; ++i) {
            if (arch->sizes[i] == 0) continue;
            size_t size = arch->sizes[i];
            size_t offset = arch->offsets[i];

            std::byte* data = (std::byte*)chunk->data;
            std::memcpy(
                data + offset + (rowToDelete * size),  // Dest
                data + offset + (lastRowIndex * size), // Src
                size
            );
        }

        Entity_id* ids = (Entity_id*)chunk->data;
        movedId = ids[lastRowIndex];
        ids[rowToDelete] = movedId;
    }

    chunk->row--; 
    return movedId;
}
/*
 * Destroys an entity, freeing its resources and updating records.
 */
void EntityManager::destroyEntity(Entity_id entity) {
    uint32_t index = entity & ENTITY_INDEX;
    if (index >= entityRecords.size()) return;

    EntityData& data = entityRecords[index];
    Chunk* chunk = data.chunk;

    freeEntityIds.push_back(index);

    Entity_id movedEntity = swapAndPopChunkRow(data.row, chunk);

    if (movedEntity != NULL_ENTITY) {
        entityRecords[movedEntity & ENTITY_INDEX].row = data.row;
    }

    tryMergeAndFreeChunk(chunk);

    entityCount--;
}
/*
 * Returns pointer to component data for given entity and component type.
 * Returns nullptr if entity does not have the component
*/
void* EntityManager::getComponentData(Entity_id entityId, ComponentMask component) {
    uint32_t index = entityId & ENTITY_INDEX;
    if (index >= entityRecords.size()) return nullptr;

    EntityData& data = entityRecords[index];
    Archetype* arch = data.archetype;

    if (((arch->componentMask >> component) & 1) == 0) {
        return nullptr; // Component not present
    }

    size_t size = arch->sizes[component];
    size_t offset = arch->offsets[component];

    std::byte* basePtr = (std::byte*)data.chunk->data;
    return basePtr + offset + (data.row * size);
}

/*
 * Adds a component to an entity, moving it to a new archetype if necessary.
 */
void EntityManager::addComponent(Entity_id entityId, ComponentMask component) {
    uint32_t index = entityId & ENTITY_INDEX;
    if (index >= entityRecords.size()) return;

    EntityData& data = entityRecords[index];
    ComponentMask newMask = data.archetype->componentMask | component;

    if (newMask == data.archetype->componentMask) {
        return; // Already has component
    }

    Archetype* newArchetype = getOrCreateArchetype(newMask);
    Chunk* newChunk = getOrCreateChunk(newArchetype);
    if (!newChunk) return; // Allocation failed

    moveEntity(data.chunk, data.row, newChunk);

    Entity_id movedEntity = swapAndPopChunkRow(data.row, data.chunk);
    if (movedEntity != NULL_ENTITY) {
        entityRecords[movedEntity & ENTITY_INDEX].row = data.row;
    }

    tryMergeAndFreeChunk(data.chunk);

    data.archetype = newArchetype;
    data.chunk = newChunk;
    data.row = newChunk->row - 1; // Last added row
}

/*
* Removes a component from an entity, moving it to a new archetype if necessary.
*/
void EntityManager::removeComponent(Entity_id entityId, ComponentMask component) {
    uint32_t index = entityId & ENTITY_INDEX;
    if (index >= entityRecords.size()) return;

    EntityData& data = entityRecords[index];
    ComponentMask newMask = data.archetype->componentMask & ~component;

    if (newMask == data.archetype->componentMask) {
        return; // Component not present
    }

    Archetype* newArchetype = getOrCreateArchetype(newMask);
    Chunk* newChunk = getOrCreateChunk(newArchetype);
    if (!newChunk) return; // Allocation failed

    moveEntity(data.chunk, data.row, newChunk);

    Entity_id movedEntity = swapAndPopChunkRow(data.row, data.chunk);
    if (movedEntity != NULL_ENTITY) {
        entityRecords[movedEntity & ENTITY_INDEX].row = data.row;
    }

    tryMergeAndFreeChunk(data.chunk);

    data.archetype = newArchetype;
    data.chunk = newChunk;
    data.row = newChunk->row - 1; // Last added row
}

std::vector<Entity_id> EntityManager::getAllEntitiesWithComponents(ComponentMask components) {
    std::vector<Entity_id> result;

    for (const auto& entityData : entityRecords) {
        if (entityData.archetype && 
            (entityData.archetype->componentMask & components) == components) {
            Entity_id* ids = (Entity_id*)entityData.chunk->data;
            result.push_back(ids[entityData.row]);
        }
    }

    return result;
}

std::vector<Archetype*>& EntityManager::getAllArchetypesWithComponent(ComponentMask component) {

    auto it = archetypeMap.find(component);
    if (it != archetypeMap.end()) {
        return it->second;
    }
    
    std::vector<Archetype*> result;
    for (const auto& archPtr : existingArchetypes) {
        if ((archPtr->componentMask & component) == component) {
            result.push_back(archPtr.get());
        }
    }

    return archetypeMap.emplace(component, result).first->second;
}