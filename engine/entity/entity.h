#pragma once
#include "../math/vector.hpp"
#include "../memory/pool_allocator.h"
#include <cstdint>
#include <functional>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/* chunk based ecs to minimize cache misses */

#define BITMASK_GENERATION 0xFFC00000 // 10 bits for generation
#define BITMASK_INDEX 0x003FFFFF      // 22 bits for index,

#define NULL_ENTITY 0xFFFFFFFF

using Entity_id = std::uint32_t; // 4 billion entities should be enough
using Transform_id = uint32_t;

using ComponentMask = uint16_t;

namespace Components {
constexpr ComponentMask Position = 1 << 0;
constexpr ComponentMask Velocity = 1 << 1;
constexpr ComponentMask Health = 1 << 2;
constexpr ComponentMask Renderable = 1 << 3;
constexpr ComponentMask AI = 1 << 4;
constexpr ComponentMask Gravity = 1 << 5; // Requires velocity
constexpr ComponentMask Transformable = 1 << 6;
constexpr ComponentMask RigidBody = 1 << 7; // Requires position and velocity
constexpr ComponentMask Collider = 1 << 8;   // Requires position
constexpr ComponentMask PhysicsMaterial = 1 << 9; // Requires Collider
} // namespace Components

inline uint8_t componentMaskToIndex(ComponentMask component) {
  uint8_t index = 0;
  while (component > 1) {
    component >>= 1;
    ++index;
  }
  return index;
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
  void *data;
  std::uint32_t row;
  std::uint32_t capacity;
  Archetype *archetype = nullptr;
  std::uint32_t indexInArchetype;
};

struct Archetype {
  ComponentMask componentMask;
  std::uint8_t componentCount;
  std::uint32_t chunkCapacity;
  std::vector<Chunk *> chunks;
  std::size_t rowSize;
  std::uint32_t offsets[sizeof(ComponentMask) * 8];
  std::size_t sizes[sizeof(ComponentMask) * 8];
};

struct EntityData {
  Archetype *archetype = nullptr;
  std::uint32_t row;
  Chunk *chunk = nullptr;
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

enum class RigidBodyType : uint8_t { Static, Dynamic, Kinematic };

struct alignas(16) RigidBody {
  mathplease::Vector4 angularVelocity;
  float mass;
  float inverseMass;
  float damping;
  RigidBodyType type;
};

enum class ColliderShape : uint8_t { Box, Sphere, Capsule, Mesh };

namespace CollisionLayers {
constexpr std::uint32_t Default = 1u << 0;
constexpr std::uint32_t WorldStatic = 1u << 1;
constexpr std::uint32_t Character = 1u << 2;
constexpr std::uint32_t Sensor = 1u << 3;
constexpr std::uint32_t All = 0xFFFFFFFFu;
} // namespace CollisionLayers

namespace ColliderBehavior {
constexpr std::uint32_t Solid = 1u << 0;
constexpr std::uint32_t Trigger = 1u << 1;
} // namespace ColliderBehavior

struct alignas(16) Collider {
  mathplease::Vector4 offset;
  std::uint32_t collisionLayer = CollisionLayers::Default;
  std::uint32_t collisionMask = CollisionLayers::All;
  std::uint32_t behaviorFlags = ColliderBehavior::Solid;
  ColliderShape shape;
  std::uint8_t reserved[3];

  union {
    mathplease::Vector4 halfExtents; // box
    float radius;                    // sphere
    struct {
      float radius;
      float halfHeight;
    } capsule;
    std::uint32_t meshId;
  };

  bool layerMaskPasses(const Collider &other) const {
    return (collisionMask & other.collisionLayer) != 0u &&
           (other.collisionMask & collisionLayer) != 0u;
  }

  bool isTrigger() const {
    return (behaviorFlags & ColliderBehavior::Trigger) != 0u;
  }

  bool isSolid() const {
    return (behaviorFlags & ColliderBehavior::Solid) != 0u;
  }
};

struct alignas(8) PhysicsMaterial {
  float friction;
  float restitution;
};

class EntityManager {
public:
  using EntityDestroyedCallback = std::function<void(Entity_id, Transform_id)>;

  EntityManager();
  ~EntityManager();
  Entity_id createEntity(ComponentMask components);
  void destroyEntity(Entity_id entityId);
  void *getComponentData(Entity_id entityId, ComponentMask component);
  void addComponent(Entity_id entityId, ComponentMask component);
  void removeComponent(Entity_id entityId, ComponentMask component);
  std::vector<Entity_id> getAllEntitiesWithComponents(ComponentMask components);
  std::vector<Archetype *> &
  getAllArchetypesWithComponent(ComponentMask component);
  void setEntityDestroyedCallback(EntityDestroyedCallback callback) {
    entityDestroyedCallback = std::move(callback);
  }

private:
  std::unordered_map<ComponentMask, std::vector<Archetype *>> archetypeMap;
  static constexpr std::size_t CHUNK_SIZE = 16 * 1024; // 16 KB
  std::vector<EntityData> entityRecords;
  std::vector<uint32_t> freeEntityIds;
  std::uint32_t entityCount;
  std::uint32_t entityCapacity;
  std::vector<std::unique_ptr<Archetype>>
      existingArchetypes;    // should move into archetype manager later or use
                             // another allocater
  std::vector<Chunk> chunks; // pointer to array of chunks
  Entity_id nextEntityId;
  EntityDestroyedCallback entityDestroyedCallback;
  void ensureEntityCapacity();
  Archetype *getOrCreateArchetype(ComponentMask components);
  Entity_id swapAndPopChunkRow(uint16_t row, Chunk *chunk);
  void tryMergeAndFreeChunk(Chunk *chunk);
  void moveEntity(Chunk *srcChunk, uint32_t srcRow, Chunk *dstChunk);
  void zeroInitializeEntityComponents(Archetype *archetype, Chunk *chunk,
                                      uint32_t row);
  void applyComponentDefaults(Archetype *archetype, Chunk *chunk, uint32_t row,
                              ComponentMask components);
  Chunk *getOrCreateChunk(Archetype *archetype);
  void *allocateChunkData();
  void releaseChunkData(void *chunkData);
  PoolAllocator chunkMetadata{sizeof(Chunk), 256};
  PoolAllocator chunkAllocator{CHUNK_SIZE, 1024};
  std::unordered_set<std::byte *> overflowChunkBlocks;
};

class ComponentRegistry {
public:
  static std::vector<ComponentInfo> &getRegistry() {
    static std::vector<ComponentInfo> registry;
    return registry;
  }

  template <typename T> static uint8_t registerType() {
    auto &registry = getRegistry();
    uint8_t id = static_cast<uint8_t>(registry.size());
    registry.push_back({sizeof(T), alignof(T)});
    return id;
  }

  static ComponentInfo getInfo(uint8_t id) {
    if (id >= getRegistry().size())
      return {0, 0};
    return getRegistry()[id];
  }
};
