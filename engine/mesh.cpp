#define VMA_IMPLEMENTATION
#include "external/vk_mem_alloc.h"

#include "mesh.h"
#include "logger.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <algorithm>
#include <cmath>


Mesh::Mesh(VkDevice device, VmaAllocator allocator, const MeshData& data)
    : m_device(device), m_allocator(allocator), m_vertexBuffer(VK_NULL_HANDLE), 
      m_indexBuffer(VK_NULL_HANDLE), m_vertexAllocation(VK_NULL_HANDLE), 
      m_indexAllocation(VK_NULL_HANDLE), m_vertexCount(0), m_indexCount(0) {
    
    if (!data.vertices.empty()) {
        createVertexBuffer(data.vertices);
        calculateBounds(data.vertices);
        m_vertexCount = static_cast<uint32_t>(data.vertices.size());
    }
    
    if (!data.indices.empty()) {
        createIndexBuffer(data.indices);
        m_indexCount = static_cast<uint32_t>(data.indices.size());
    }
    
    LOG_INFO("MESH", "Created mesh with {} vertices, {} indices", m_vertexCount, m_indexCount);
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::cleanup() {
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_indexBuffer, m_indexAllocation);
        m_indexBuffer = VK_NULL_HANDLE;
        m_indexAllocation = VK_NULL_HANDLE;
    }
    
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_vertexBuffer, m_vertexAllocation);
        m_vertexBuffer = VK_NULL_HANDLE;
        m_vertexAllocation = VK_NULL_HANDLE;
    }
}

void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    
    VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
                            VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VmaAllocationInfo stagingAllocInfoResult;
    vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, 
                    &stagingBuffer, &stagingAllocation, &stagingAllocInfoResult);
    
    // Copy vertex data to staging buffer
    std::memcpy(stagingAllocInfoResult.pMappedData, vertices.data(), bufferSize);
    
    // Create vertex buffer
    VkBufferCreateInfo vertexBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    vertexBufferInfo.size = bufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo vertexAllocInfo = {};
    vertexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    vertexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    
    vmaCreateBuffer(m_allocator, &vertexBufferInfo, &vertexAllocInfo, 
                    &m_vertexBuffer, &m_vertexAllocation, nullptr);
    
    // Copy from staging buffer to vertex buffer
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
    
    // Cleanup staging buffer
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
}

void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    
    // Create staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    
    VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
                            VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    VmaAllocationInfo stagingAllocInfoResult;
    vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo, 
                    &stagingBuffer, &stagingAllocation, &stagingAllocInfoResult);
    
    // Copy index data to staging buffer
    std::memcpy(stagingAllocInfoResult.pMappedData, indices.data(), bufferSize);
    
    // Create index buffer
    VkBufferCreateInfo indexBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    indexBufferInfo.size = bufferSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo indexAllocInfo = {};
    indexAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    indexAllocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    
    vmaCreateBuffer(m_allocator, &indexBufferInfo, &indexAllocInfo, 
                    &m_indexBuffer, &m_indexAllocation, nullptr);
    
    // Copy from staging buffer to index buffer
    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);
    
    // Cleanup staging buffer
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
}

void Mesh::calculateBounds(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) {
        m_bounds = BoundingBox();
        return;
    }
    
    Vector3 min = vertices[0].pos;
    Vector3 max = vertices[0].pos;
    
    for (const auto& vertex : vertices) {
        min.x = std::min(min.x, vertex.pos.x);
        min.y = std::min(min.y, vertex.pos.y);
        min.z = std::min(min.z, vertex.pos.z);
        
        max.x = std::max(max.x, vertex.pos.x);
        max.y = std::max(max.y, vertex.pos.y);
        max.z = std::max(max.z, vertex.pos.z);
    }
    
    m_bounds = BoundingBox(min, max);
}

void Mesh::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    // For now, this is a simplified version - in a real implementation,
    // you'd want to use a command pool and queue for transfers
    LOG_WARN("MESH", "copyBuffer not fully implemented - needs command buffer setup");
    // TODO: Implement proper buffer copying with command buffers
}

void Mesh::bind(VkCommandBuffer cmd) {
    VkBuffer vertexBuffers[] = { m_vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

void Mesh::draw(VkCommandBuffer cmd, uint32_t instanceCount) {
    if (m_indexCount > 0) {
        drawIndexed(cmd, instanceCount);
    } else {
        vkCmdDraw(cmd, m_vertexCount, instanceCount, 0, 0);
    }
}

void Mesh::drawIndexed(VkCommandBuffer cmd, uint32_t instanceCount) {
    vkCmdDrawIndexed(cmd, m_indexCount, instanceCount, 0, 0, 0);
}

VkPipelineVertexInputStateCreateInfo Mesh::getVertexInputState() {
    static VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    static auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    return vertexInputInfo;
}

void Mesh::updateVertices(const std::vector<Vertex>& newVertices) {
    // Cleanup old buffer
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_vertexBuffer, m_vertexAllocation);
    }
    
    // Create new buffer
    createVertexBuffer(newVertices);
    calculateBounds(newVertices);
    m_vertexCount = static_cast<uint32_t>(newVertices.size());
}

void Mesh::updateIndices(const std::vector<uint32_t>& newIndices) {
    // Cleanup old buffer
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_indexBuffer, m_indexAllocation);
    }
    
    // Create new buffer
    createIndexBuffer(newIndices);
    m_indexCount = static_cast<uint32_t>(newIndices.size());
}

// Static factory methods
Mesh Mesh::createQuad(VkDevice device, VmaAllocator allocator) {
    MeshData data;
    data.vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}  // top-left
    };
    
    data.indices = { 0, 1, 2, 2, 3, 0 };
    
    return Mesh(device, allocator, data);
}

Mesh Mesh::createTriangle(VkDevice device, VmaAllocator allocator) {
    MeshData data;
    data.vertices = {
        {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}}, // bottom
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}  // top-left
    };
    
    data.indices = { 0, 1, 2 };
    
    return Mesh(device, allocator, data);
}

Mesh Mesh::createCube(VkDevice device, VmaAllocator allocator) {
    MeshData data;
    data.vertices = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
        
        // Back face
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}}
    };
    
    data.indices = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        7, 3, 0, 0, 4, 7,
        // Right face
        1, 5, 6, 6, 2, 1,
        // Top face
        3, 2, 6, 6, 7, 3,
        // Bottom face
        0, 1, 5, 5, 4, 0
    };
    
    return Mesh(device, allocator, data);
}

Mesh Mesh::createSphere(VkDevice device, VmaAllocator allocator, int subdivisions) {
    MeshData data;
    
    // Simple sphere generation using UV sphere method
    const float PI = 3.14159265359f;
    
    for (int lat = 0; lat <= subdivisions; ++lat) {
        float theta = lat * PI / subdivisions;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (int lon = 0; lon <= subdivisions; ++lon) {
            float phi = lon * 2 * PI / subdivisions;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            Vertex vertex;
            vertex.pos.x = cosPhi * sinTheta;
            vertex.pos.y = cosTheta;
            vertex.pos.z = sinPhi * sinTheta;
            
            vertex.normal = vertex.pos; // For a unit sphere, position = normal
            
            vertex.uv.x = 1.0f - (float)lon / subdivisions;
            vertex.uv.y = 1.0f - (float)lat / subdivisions;
            
            // Simple color based on position
            vertex.colour.x = (vertex.pos.x + 1.0f) * 0.5f;
            vertex.colour.y = (vertex.pos.y + 1.0f) * 0.5f;
            vertex.colour.z = (vertex.pos.z + 1.0f) * 0.5f;
            
            data.vertices.push_back(vertex);
        }
    }
    
    // Generate indices
    for (int lat = 0; lat < subdivisions; ++lat) {
        for (int lon = 0; lon < subdivisions; ++lon) {
            int first = lat * (subdivisions + 1) + lon;
            int second = first + subdivisions + 1;
            
            data.indices.push_back(first);
            data.indices.push_back(second);
            data.indices.push_back(first + 1);
            
            data.indices.push_back(second);
            data.indices.push_back(second + 1);
            data.indices.push_back(first + 1);
        }
    }
    
    return Mesh(device, allocator, data);
}

Mesh Mesh::createPrimitive(VkDevice device, VmaAllocator allocator, PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Quad:
            return createQuad(device, allocator);
        case PrimitiveType::Triangle:
            return createTriangle(device, allocator);
        case PrimitiveType::Cube:
            return createCube(device, allocator);
        case PrimitiveType::Sphere:
            return createSphere(device, allocator);
        default:
            LOG_WARN("MESH", "Unknown primitive type, falling back to triangle");
            return createTriangle(device, allocator);
    }
}

