#include "external/vk_mem_alloc.h"

#include "mesh.h"
#include "logger.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <cstring>
#include <algorithm>
#include <cmath>


Mesh::Mesh(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, const MeshData& data)
    : m_device(device), m_allocator(allocator), m_commandPool(commandPool), m_graphicsQueue(graphicsQueue),
      m_vertexBuffer(VK_NULL_HANDLE), m_indexBuffer(VK_NULL_HANDLE), 
      m_vertexAllocation(VK_NULL_HANDLE), m_indexAllocation(VK_NULL_HANDLE), 
      m_vertexCount(0), m_indexCount(0) {
    
    if (!data.vertices.empty()) {
        createVertexBuffer(data.vertices);
        calculateBounds(data.vertices);
        m_vertexCount = static_cast<uint32_t>(data.vertices.size());
        createIndexBuffer(data.indices);
        m_indexCount = static_cast<uint32_t>(data.indices.size());
        LOG_INFO("MESH", "Created mesh with {} vertices, {} indices", m_vertexCount, m_indexCount);
    }
    
}

Mesh::~Mesh() {
    cleanup();
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        cleanup(); // Clean up existing resources in 'this'

        m_device = other.m_device;
        m_allocator = other.m_allocator;
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        m_vertexAllocation = other.m_vertexAllocation;
        m_indexAllocation = other.m_indexAllocation;
        m_vertexCount = other.m_vertexCount;
        m_indexCount = other.m_indexCount;

        other.m_vertexBuffer = VK_NULL_HANDLE;
        other.m_indexBuffer = VK_NULL_HANDLE;
        other.m_vertexAllocation = VK_NULL_HANDLE;
        other.m_indexAllocation = VK_NULL_HANDLE;
    }
    return *this;
}

Mesh::Mesh(Mesh&& other) noexcept {
    // Transfer ownership of handles
    this->m_vertexBuffer = other.m_vertexBuffer;
    this->m_indexBuffer = other.m_indexBuffer;
    this->m_vertexAllocation = other.m_vertexAllocation;
    this->m_indexAllocation = other.m_indexAllocation;
    this->m_device = other.m_device;
    this->m_allocator = other.m_allocator;
    this->m_commandPool = other.m_commandPool;
    this->m_graphicsQueue = other.m_graphicsQueue;
    this->m_vertexCount = other.m_vertexCount;
    this->m_indexCount = other.m_indexCount;
    this->m_bounds = other.m_bounds;

    // "Null out" the source object so its destructor does nothing
    other.m_vertexBuffer = VK_NULL_HANDLE;
    other.m_indexBuffer = VK_NULL_HANDLE;
    other.m_vertexAllocation = VK_NULL_HANDLE;
    other.m_indexAllocation = VK_NULL_HANDLE;
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
    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize, m_commandPool, m_graphicsQueue, m_device);
    
    // Cleanup staging buffer
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
    LOG_INFO("MESH_VB", "Created Vertex buffer succesfully");
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
    copyBuffer(stagingBuffer, m_indexBuffer, bufferSize, m_commandPool, m_graphicsQueue, m_device);
    
    // Cleanup staging buffer
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
}

void Mesh::calculateBounds(const std::vector<Vertex>& vertices) {
    if (vertices.empty()) {
        m_bounds = BoundingBox();
        return;
    }
    
    glm::vec3 min = vertices[0].pos;
    glm::vec3 max = vertices[0].pos;
    
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

void Mesh::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, 
        VkCommandPool commandPool, VkQueue graphicsQueue, VkDevice device) {
    
    // Validate input parameters
    if (srcBuffer == VK_NULL_HANDLE || dstBuffer == VK_NULL_HANDLE || size == 0) {
        LOG_ERR("MESH", "copyBuffer: Invalid buffer handles or size");
        return;
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd;
    VkResult result = vkAllocateCommandBuffers(device, &allocInfo, &cmd);
    if (result != VK_SUCCESS) {
        LOG_ERR("MESH", "copyBuffer: Failed to allocate command buffer: {}", static_cast<int>(result));
        return;
    }

    // Create a fence for synchronization
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0; // Start unsignaled
    
    VkFence copyFence;
    result = vkCreateFence(device, &fenceInfo, nullptr, &copyFence);
    if (result != VK_SUCCESS) {
        LOG_ERR("MESH", "copyBuffer: Failed to create fence: {}", static_cast<int>(result));
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        return;
    }

    // Begin recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(cmd, &beginInfo);
    if (result != VK_SUCCESS) {
        LOG_ERR("MESH", "copyBuffer: Failed to begin command buffer: {}", static_cast<int>(result));
        vkDestroyFence(device, copyFence, nullptr);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        return;
    }

    // Copy region
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);

    result = vkEndCommandBuffer(cmd);
    if (result != VK_SUCCESS) {
        LOG_ERR("MESH", "copyBuffer: Failed to end command buffer: {}", static_cast<int>(result));
        vkDestroyFence(device, copyFence, nullptr);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        return;
    }

    // Submit with fence
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, copyFence);
    if (result != VK_SUCCESS) {
        LOG_ERR("MESH", "copyBuffer: Failed to submit command buffer: {}", static_cast<int>(result));
        vkDestroyFence(device, copyFence, nullptr);
        vkFreeCommandBuffers(device, commandPool, 1, &cmd);
        return;
    }

    // Wait for completion with fence
    result = vkWaitForFences(device, 1, &copyFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        LOG_ERR("MESH", "copyBuffer: Failed to wait for fence: {}", static_cast<int>(result));
    }

    // Cleanup
    vkDestroyFence(device, copyFence, nullptr);
    vkFreeCommandBuffers(device, commandPool, 1, &cmd);

    LOG_INFO("MESH", "Buffer copy completed successfully");
}

void Mesh::bind(VkCommandBuffer cmd) {
    VkBuffer *vertexBuffers = &m_vertexBuffer;
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
Mesh Mesh::createQuad(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue) {
    MeshData data;
    data.vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // bottom-left
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}}, // bottom-right
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}  // top-left
    };
    
    data.indices = { 0, 1, 2, 2, 3, 0 };
    
    return Mesh(device, allocator, commandPool, graphicsQueue, data);
}

Mesh Mesh::createTriangle(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue) {
    MeshData data;
    data.vertices = {
        {{ 0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}}, // bottom
        {{ 0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // top-right
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}  // top-left
    };
    
    data.indices = { 0, 1, 2 };
    
    return Mesh(device, allocator, commandPool, graphicsQueue, data);
}

Mesh Mesh::createCube(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue) {
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
    
    return Mesh(device, allocator, commandPool, graphicsQueue, data);
}

Mesh Mesh::createSphere(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, int subdivisions) {
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
    
    return Mesh(device, allocator, commandPool, graphicsQueue, data);
}

Mesh Mesh::createPrimitive(VkDevice device, VmaAllocator allocator, VkCommandPool commandPool, VkQueue graphicsQueue, PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Quad:
            return createQuad(device, allocator, commandPool, graphicsQueue);
        case PrimitiveType::Triangle:
            return createTriangle(device, allocator, commandPool, graphicsQueue);
        case PrimitiveType::Cube:
            return createCube(device, allocator, commandPool, graphicsQueue);
        case PrimitiveType::Sphere:
            return createSphere(device, allocator, commandPool, graphicsQueue);
        default:
            LOG_WARN("MESH", "Unknown primitive type, falling back to triangle");
            return createTriangle(device, allocator, commandPool, graphicsQueue);
    }
}

