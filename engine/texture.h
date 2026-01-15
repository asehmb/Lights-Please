#pragma once


#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "external/vk_mem_alloc.h"


class Texture {
public:
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    
    // Image dimensions
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    
    // Image data (temporary storage during loading)
    unsigned char* pixels = nullptr;

    // Methods used by renderer's createTexture function
    void createImage(VkDevice device, VmaAllocator allocator, VkPhysicalDevice physicalDevice, const char* imagePath);
    void createStagingBuffer(VmaAllocator allocator, VkBuffer& stagingBuffer, VmaAllocation& stagingAllocation);
    void createViewAndSampler(VkDevice device);
    
    // Original method (for compatibility)
    void create(VkDevice device, VmaAllocator allocator, VkPhysicalDevice physicalDevice,
                VkCommandPool commandPool, VkQueue graphicsQueue,
                const char* imagePath);
    void cleanup(VkDevice device, VmaAllocator allocator);
};