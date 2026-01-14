#pragma once

#include "math/vector.hpp"
#include "vulkan/vulkan_core.h" 
#define VMA_INCLUDE_ONLY
#include "external/vk_mem_alloc.h"


struct alignas(16) GlobalUniforms {
    mathplease::Matrix4 model;
    mathplease::Matrix4 view;
    mathplease::Matrix4 proj;
};

class UBO {
public:
    UBO() = default;
    ~UBO() = default;
    GlobalUniforms data;
    void* mapped = nullptr;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;

    void create(VkDevice device, VmaAllocator allocator);
    void update(mathplease::Matrix4 model, mathplease::Matrix4 view, mathplease::Matrix4 proj);
    void update(mathplease::Matrix4 view, mathplease::Matrix4 proj);
    void cleanup(VmaAllocator allocator);
};