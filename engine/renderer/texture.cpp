#include "renderer.h"
#include "vulkan/vulkan_core.h"
#define STB_IMAGE_IMPLEMENTATION
#include "texture.h"
#include <stdexcept>
#include "../external/stb_image.h"
#include "../logger.h"
#include <cstring>

void Texture::createImage(VkDevice device, VmaAllocator allocator, VkPhysicalDevice physicalDevice, const char* imagePath) {
    // Load image using stb_image
    int texWidth, texHeight, texChannels;
    pixels = stbi_load(imagePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    if (!pixels) {
        LOG_ERR("TEXTURE", "Failed to load texture image: {}", imagePath);
        throw std::runtime_error(std::string("Failed to load texture image ") + imagePath + "!");
    }
    
    width = static_cast<uint32_t>(texWidth);
    height = static_cast<uint32_t>(texHeight);
    channels = 4; // STBI_rgb_alpha forces 4 channels
    
    LOG_INFO("TEXTURE", "Loaded texture: {} ({}x{}, {} channels)", imagePath, width, height, channels);
    
    // Create VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    
    if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
        stbi_image_free(pixels);
        pixels = nullptr;
        LOG_ERR("TEXTURE", "Failed to create VkImage");
        throw std::runtime_error("Failed to create VkImage");
    }
    
    LOG_INFO("TEXTURE", "Created VkImage successfully");
}

void Texture::createStagingBuffer(VmaAllocator allocator, VkBuffer& stagingBuffer, VmaAllocation& stagingAllocation) {
    if (!pixels) {
        LOG_ERR("TEXTURE", "No pixel data available for staging buffer");
        throw std::runtime_error("No pixel data available for staging buffer");
    }
    
    VkDeviceSize imageSize = width * height * channels;
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
        LOG_ERR("TEXTURE", "Failed to create staging buffer");
        throw std::runtime_error("Failed to create staging buffer");
    }
    
    // Copy pixel data to staging buffer
    void* data;
    vmaMapMemory(allocator, stagingAllocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator, stagingAllocation);
    
    // Free the original pixel data now that it's copied to staging buffer
    stbi_image_free(pixels);
    pixels = nullptr;
    
    LOG_INFO("TEXTURE", "Created staging buffer and copied pixel data");
}

void Texture::createViewAndSampler(VkDevice device) {
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        LOG_ERR("TEXTURE", "Failed to create image view");
        throw std::runtime_error("Failed to create image view");
    }
    
    // Create sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        LOG_ERR("TEXTURE", "Failed to create sampler");
        throw std::runtime_error("Failed to create sampler");
    }
    
    LOG_INFO("TEXTURE", "Created image view and sampler successfully");
}

void Texture::create(VkDevice device, VmaAllocator allocator, VkPhysicalDevice physicalDevice,
                     VkCommandPool commandPool, VkQueue graphicsQueue,
                     const char* imagePath) {
    // Load image using stb_image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(imagePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error(std::string("Failed to load texture image ") + imagePath + "!");
    }

    // Create staging buffer


}


void Texture::cleanup(VkDevice device, VmaAllocator allocator) {
    if (sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, sampler, nullptr);
        sampler = VK_NULL_HANDLE;
    }
    if (imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }
    if (image != VK_NULL_HANDLE) {
        vmaDestroyImage(allocator, image, allocation);
        image = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
    if (pixels) {
        stbi_image_free(pixels);
        pixels = nullptr;
    }
}