
#include "ubo.h"
#include <cstring>
#include <stdexcept>

void UBO::create(VkDevice device, VmaAllocator allocator) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(GlobalUniforms);
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // single queue family

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create UBO buffer!");
    }

    // Map the buffer memory
    vmaMapMemory(allocator, allocation, &mapped);
}

void UBO::update(mathplease::Matrix4 model, mathplease::Matrix4 view, mathplease::Matrix4 proj) {
    data.model = model;
    data.view = view;
    data.proj = proj;
    data.proj(1, 1) *= -1; // Adjust for Vulkan's coordinate system

    // Copy data to mapped memory
    std::memcpy(mapped, &data, sizeof(GlobalUniforms));
}

// Overloaded update function for view and proj only
void UBO::update(mathplease::Matrix4 view, mathplease::Matrix4 proj) {
    // proj(1, 1) *= -1;
    mathplease::Matrix4 identity = mathplease::Matrix4::identity();
    data.model = identity;
    data.view = view;
    data.proj = proj;
    // data.proj(1, 1) *= -1; 

    std::memcpy(mapped, &data, sizeof(GlobalUniforms));
}

void UBO::cleanup(VmaAllocator allocator) {
    if (mapped) {
        vmaUnmapMemory(allocator, allocation);
        mapped = nullptr;
    }
    if (buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(allocator, buffer, allocation);
        buffer = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}