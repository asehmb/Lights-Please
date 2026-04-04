
#include "entity.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

namespace {
constexpr Entity_id kGenerationIncrement = BITMASK_INDEX + 1;
}

uint32_t alignUp(uint32_t offset, size_t alignment) {
    return (offset + (alignment - 1)) & ~(alignment - 1);
}

EntityManager::EntityManager() {
    entityCount = 0;
    entityCapacity = 4194304; // preallocate for 4 million entities 2^22
    nextEntityId = 0;
    entityRecords.reserve(4194304);

    // instantiate registry
    ComponentRegistry::registerType<Position>();
    ComponentRegistry::registerType<Velocity>();
    ComponentRegistry::registerType<Health>();
    ComponentRegistry::registerType<Renderable>();
    ComponentRegistry::registerType<AI>();
    ComponentRegistry::registerType<Gravity>();
    ComponentRegistry::registerType<Transformable>();
    ComponentRegistry::registerType<RigidBody>();
    ComponentRegistry::registerType<Collider>();
    ComponentRegistry::registerType<PhysicsMaterial>();
}

EntityManager::~EntityManager() {
    for (auto& arch : existingArchetypes) {
        for (auto& chunk : arch->chunks) {
            releaseChunkData(chunk->data);
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
    zeroInitializeEntityComponents(archetype, chunk, row);
    applyComponentDefaults(archetype, chunk, row, components);

    // Store entity location
    EntityData entityData;
    entityData.archetype = archetype;
    entityData.chunk = chunk;
    entityData.row = row;

    chunk->row++; // advance row for next entity

    Entity_id finalId;
    if (freeEntityIds.empty()) {
        finalId = nextEntityId++;
        entityData.id = finalId;
        entityRecords.push_back(entityData);
    } else {
        Entity_id reusedId = freeEntityIds.back();
        freeEntityIds.pop_back();
        finalId = reusedId & BITMASK_INDEX | // get index
            (((reusedId & BITMASK_GENERATION) + kGenerationIncrement) // increment generation
             & BITMASK_GENERATION); // wrap around generation
        entityData.id = finalId;
        entityRecords[(reusedId & BITMASK_INDEX)] = entityData;
    }
    entityCount++;

    // record id in chunk
    Entity_id* ids = (Entity_id*)chunk->data;
    ids[row] = finalId;


    return finalId;

}

void EntityManager::zeroInitializeEntityComponents(Archetype* archetype, Chunk* chunk,
                                                   uint32_t row) {
    if (!archetype || !chunk) {
        return;
    }

    std::byte* chunkData = static_cast<std::byte*>(chunk->data);
    for (int i = 0; i < 16; ++i) {
        if ((archetype->componentMask & (1u << i)) == 0u) {
            continue;
        }

        const size_t size = archetype->sizes[i];
        if (size == 0) {
            continue;
        }

        const size_t offset = archetype->offsets[i];
        std::byte* dst = chunkData + offset + (row * size);
        std::memset(dst, 0, size);
    }
}

void EntityManager::applyComponentDefaults(Archetype* archetype, Chunk* chunk,
                                           uint32_t row,
                                           ComponentMask components) {
    if (!archetype || !chunk) {
        return;
    }

    if ((components & Components::Collider) != 0u) {
        auto* collider = reinterpret_cast<Collider*>(
            static_cast<std::byte*>(chunk->data) +
            archetype->offsets[componentMaskToIndex(Components::Collider)] +
            (row * archetype->sizes[componentMaskToIndex(Components::Collider)]));
        collider->collisionLayer = CollisionLayers::Default;
        collider->collisionMask = CollisionLayers::All;
        collider->behaviorFlags = ColliderBehavior::Solid;
        collider->shape = ColliderShape::Box;
    }
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
void *EntityManager::allocateChunkData() {
    void *chunkData = chunkAllocator.allocate();
    if (chunkData) {
        return chunkData;
    }

    std::byte *overflowBlock = new (std::nothrow) std::byte[CHUNK_SIZE];
    if (!overflowBlock) {
        return nullptr;
    }
    overflowChunkBlocks.insert(overflowBlock);
    return overflowBlock;
}

void EntityManager::releaseChunkData(void *chunkData) {
    if (!chunkData) {
        return;
    }

    auto overflowIt = overflowChunkBlocks.find(static_cast<std::byte *>(chunkData));
    if (overflowIt != overflowChunkBlocks.end()) {
        delete[] *overflowIt;
        overflowChunkBlocks.erase(overflowIt);
        return;
    }

    chunkAllocator.deallocate(chunkData);
}

Chunk* EntityManager::getOrCreateChunk(Archetype* archetype) {
    // 1. Check existing last chunk
    if (!archetype->chunks.empty()) {
        Chunk* last = archetype->chunks.back();
        if (last->row < last->capacity) {
            return last;
        }
    }

    // 2. Allocate
    void* chunkData = allocateChunkData();
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
    Archetype* srcArch = srcChunk->archetype;
    Archetype* dstArch = dstChunk->archetype;
    uint32_t dstRow = dstChunk->row;

    zeroInitializeEntityComponents(dstArch, dstChunk, dstRow);
    applyComponentDefaults(dstArch, dstChunk, dstRow, dstArch->componentMask);

    // 1. Copy Components
    for (int i = 0; i < 16; ++i) {
        if (srcArch->sizes[i] == 0 || dstArch->sizes[i] == 0) continue;
        size_t srcSize = srcArch->sizes[i];
        size_t dstSize = dstArch->sizes[i];
        size_t copySize = std::min(srcSize, dstSize);
        size_t srcOffset = srcArch->offsets[i];
        size_t dstOffset = dstArch->offsets[i];

        std::byte* srcPtr = (std::byte*)srcChunk->data + srcOffset + (srcRow * srcSize);
        std::byte* dstPtr = (std::byte*)dstChunk->data + dstOffset + (dstRow * dstSize);
        std::memcpy(dstPtr, srcPtr, copySize);
    }

    // 2. Copy ID
    Entity_id* srcIds = (Entity_id*)srcChunk->data;
    Entity_id* dstIds = (Entity_id*)dstChunk->data;
    Entity_id id = srcIds[srcRow];
    dstIds[dstRow] = id;

    // 3. Update Global Record
    entityRecords[id & BITMASK_INDEX].chunk = dstChunk;
    entityRecords[id & BITMASK_INDEX].row = dstRow;

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
        releaseChunkData(chunk->data);
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
    uint32_t index = entity & BITMASK_INDEX;
    if (index >= entityRecords.size()) return;

    EntityData& data = entityRecords[index];
    if (data.id != entity || !data.chunk || !data.archetype) return;

    Transform_id transformHandle = NULL_ENTITY;
    if ((data.archetype->componentMask & Components::Transformable) != 0) {
        auto* transformable = static_cast<Transformable*>(
            getComponentData(entity, Components::Transformable));
        if (transformable) {
            transformHandle = transformable->handle;
        }
    }
    if (entityDestroyedCallback) {
        entityDestroyedCallback(entity, transformHandle);
    }

    Chunk* chunk = data.chunk;
    const uint32_t row = data.row;

    freeEntityIds.push_back(entity);

    Entity_id movedEntity = swapAndPopChunkRow(row, chunk);

    if (movedEntity != NULL_ENTITY) {
        entityRecords[movedEntity & BITMASK_INDEX].row = row;
    }

    tryMergeAndFreeChunk(chunk);

    data = {};
    data.id = NULL_ENTITY;
    if (entityCount > 0) {
        entityCount--;
    }
}
/*
 * Returns pointer to component data for given entity and component type.
 * Returns nullptr if entity does not have the component
*/
void* EntityManager::getComponentData(Entity_id entityId, ComponentMask component) {
    uint32_t index = entityId & BITMASK_INDEX;
    if (index >= entityRecords.size()) return nullptr;

    EntityData& data = entityRecords[index];
    if (data.id != entityId || !data.chunk || !data.archetype) {
        return nullptr;
    }
    Archetype* arch = data.archetype;

    if ((arch->componentMask & component) == 0) {
        return nullptr; // Component not present
    }

    uint8_t componentIndex = componentMaskToIndex(component);
    size_t size = arch->sizes[componentIndex];
    size_t offset = arch->offsets[componentIndex];

    std::byte* basePtr = (std::byte*)data.chunk->data;
    return basePtr + offset + (data.row * size);
}

/*
 * Adds a component to an entity, moving it to a new archetype if necessary.
 */
void EntityManager::addComponent(Entity_id entityId, ComponentMask component) {
    uint32_t index = entityId & BITMASK_INDEX;
    if (index >= entityRecords.size()) return;

    EntityData& data = entityRecords[index];
    if (data.id != entityId || !data.chunk || !data.archetype) return;
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
        entityRecords[movedEntity & BITMASK_INDEX].row = data.row;
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
    uint32_t index = entityId & BITMASK_INDEX;
    if (index >= entityRecords.size()) return;

    EntityData& data = entityRecords[index];
    if (data.id != entityId || !data.chunk || !data.archetype) return;
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
        entityRecords[movedEntity & BITMASK_INDEX].row = data.row;
    }

    tryMergeAndFreeChunk(data.chunk);

    data.archetype = newArchetype;
    data.chunk = newChunk;
    data.row = newChunk->row - 1; // Last added row
}

std::vector<Entity_id> EntityManager::getAllEntitiesWithComponents(ComponentMask components) {
    std::vector<Entity_id> result;

    for (const auto& entityData : entityRecords) {
        if (entityData.id != NULL_ENTITY && entityData.chunk && entityData.archetype &&
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
