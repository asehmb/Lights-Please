#pragma once
#include <cstdint>
#include <sys/types.h>
#include <vector>
#include "../memory/pool_allocator.h"
#include "../math/vector.hpp"
#include <unordered_map>

// chunk based ecs to minimize cache misses
#define BITMASK_GENERATION 0xFFC00000 // 22 bits for index, 10 bits for generation
#define BITMASK_INDEX      0x003FFFFF

#define NULL_ENTITY 0xFFFFFFFF

using Entity_id = std::uint32_t; // 4 billion entities should be enough
using Transform_id = uint32_t;

using ComponentMask = uint16_t;


namespace Components {
    constexpr ComponentMask Position   = 1 << 0;
    constexpr ComponentMask Velocity   = 1 << 1;
    constexpr ComponentMask Health     = 1 << 2;
    constexpr ComponentMask Renderable = 1 << 3;
    constexpr ComponentMask AI         = 1 << 4;
    constexpr ComponentMask Gravity    = 1 << 5; // Requires velocity and position
    constexpr ComponentMask Transformable = 1 << 6;
}
struct Entity {
    Entity_id id;
};


struct ComponentInfo {
    std::size_t size;
    std::size_t offset;
};

struct Archetype;

struct Chunk {
    void* data;
    std::uint32_t row;
    std::uint32_t capacity;
    Archetype* archetype = nullptr;
    std::uint32_t indexInArchetype;
};

struct Archetype {
    ComponentMask componentMask;
    std::uint8_t componentCount;
    std::uint32_t chunkCapacity;
    std::vector<Chunk*> chunks;
    std::size_t rowSize;
    std::uint32_t offsets[sizeof(ComponentMask)*8]; 
    std::size_t sizes[sizeof(ComponentMask)*8];
};

struct EntityData {
    Archetype* archetype = nullptr;
    std::uint32_t row;
    Chunk* chunk = nullptr;
    Entity_id id;
};

struct alignas(16) Position {
    mathplease::Vector4 value; // vector 4 for alignment
};

struct alignas(16) Velocity {
    mathplease::Vector4 value; // vector 4 for alignment
};

struct alignas(8) Health {
    int current;
    int max;
};
struct alignas(8) Renderable {
    std::uint32_t meshId;
    std::uint32_t materialId;
};
struct alignas(8) AI {
    uint8_t state;
    uint8_t type;
    float aggressionLevel;
};
struct Gravity {};

struct alignas(4) Transformable {
    Transform_id handle;
};


class EntityManager {
public:
    EntityManager();
    ~EntityManager();
    Entity_id createEntity(ComponentMask components);
    void destroyEntity(Entity_id entityId);
    void* getComponentData(Entity_id entityId, ComponentMask component);
    void addComponent(Entity_id entityId, ComponentMask component);
    void removeComponent(Entity_id entityId, ComponentMask component);
    std::vector<Entity_id> getAllEntitiesWithComponents(ComponentMask components);
    std::vector<Archetype*>& getAllArchetypesWithComponent(ComponentMask component);
private:
    std::unordered_map<ComponentMask, std::vector<Archetype*>> archetypeMap;
    static constexpr std::size_t CHUNK_SIZE = 16 * 1024 ; // 16 KB
    std::vector<EntityData> entityRecords;
    std::vector<uint32_t> freeEntityIds;
    std::uint32_t entityCount;
    std::uint32_t entityCapacity;
    std::vector<std::unique_ptr<Archetype>> existingArchetypes; // should move into archetype manager later or use another allocater
    std::vector<Chunk> chunks; // pointer to array of chunks
    Entity_id nextEntityId;
    void ensureEntityCapacity();
    Archetype* getOrCreateArchetype(ComponentMask components);
    Entity_id swapAndPopChunkRow(uint16_t row, Chunk* chunk);
    void tryMergeAndFreeChunk(Chunk* chunk);
    void moveEntity(Chunk* srcChunk, uint32_t srcRow, Chunk* dstChunk);
    Chunk* getOrCreateChunk(Archetype* archetype);
    PoolAllocator chunkMetadata{ sizeof(Chunk), 256 };
    PoolAllocator chunkAllocator{ CHUNK_SIZE, 1024 };
};


class ComponentRegistry {
public:
    static std::vector<ComponentInfo>& getRegistry() {
        static std::vector<ComponentInfo> registry;
        return registry;
    }

    template <typename T>
    static uint8_t registerType() {
        auto& registry = getRegistry();
        uint8_t id = static_cast<uint8_t>(registry.size());
        registry.push_back({ sizeof(T), alignof(T) });
        return id;
    }
    
    static ComponentInfo getInfo(uint8_t id) {
        if (id >= getRegistry().size()) return {0, 0};
        return getRegistry()[id];
    }
};