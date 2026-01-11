#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "geometry.h"
#include <vector>
#ifndef VMA_INCLUDE_ONLY
#define VMA_INCLUDE_ONLY
#endif
#include "external/vk_mem_alloc.h"

enum class PrimitiveType {
    Quad,
    Cube,
    Sphere,
    Triangle
};

struct BoundingBox {
    Vector3 min;
    Vector3 max;
    
    BoundingBox() : min(0, 0, 0), max(0, 0, 0) {}
    BoundingBox(const Vector3& minPoint, const Vector3& maxPoint) : min(minPoint), max(maxPoint) {}
    
    Vector3 getCenter() const { return (min + max) * 0.5f; }
    Vector3 getSize() const { return max - min; }
};

class Mesh {
public:
    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    // Construction
    Mesh(VkDevice device, VmaAllocator allocator, const MeshData& data);
    ~Mesh();

    // Static factory methods for primitive creation
    static Mesh createQuad(VkDevice device, VmaAllocator allocator);
    static Mesh createCube(VkDevice device, VmaAllocator allocator);
    static Mesh createSphere(VkDevice device, VmaAllocator allocator, int subdivisions = 16);
    static Mesh createTriangle(VkDevice device, VmaAllocator allocator);
    static Mesh createPrimitive(VkDevice device, VmaAllocator allocator, PrimitiveType type);

    // Rendering
    void bind(VkCommandBuffer cmd);
    void draw(VkCommandBuffer cmd, uint32_t instanceCount = 1);
    void drawIndexed(VkCommandBuffer cmd, uint32_t instanceCount = 1);

    // Data access
    const BoundingBox& getBounds() const { return m_bounds; }
    size_t getVertexCount() const { return m_vertexCount; }
    size_t getIndexCount() const { return m_indexCount; }
    
    // Pipeline state info
    VkPipelineVertexInputStateCreateInfo getVertexInputState();

    // Memory management
    void updateVertices(const std::vector<Vertex>& newVertices);
    void updateIndices(const std::vector<uint32_t>& newIndices);
    void cleanup();

private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    
    VkBuffer m_vertexBuffer;
    VkBuffer m_indexBuffer;
    VmaAllocation m_vertexAllocation;
    VmaAllocation m_indexAllocation;
    
    uint32_t m_vertexCount;
    uint32_t m_indexCount;
    BoundingBox m_bounds;

    // Helper methods
    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
    void calculateBounds(const std::vector<Vertex>& vertices);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
};