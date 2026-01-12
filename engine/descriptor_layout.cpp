#include "descriptor_layout.h"
#include "logger.h"

// Static member definitions
VkDescriptorSetLayout DescriptorLayouts::s_globalLayout = VK_NULL_HANDLE;
VkDescriptorSetLayout DescriptorLayouts::s_materialLayout = VK_NULL_HANDLE;
VkDescriptorSetLayout DescriptorLayouts::s_textureLayout = VK_NULL_HANDLE;
bool DescriptorLayouts::s_initialized = false;

void DescriptorLayouts::init(VkDevice device) {
    if (s_initialized) {
        LOG_WARN("DESCRIPTOR_LAYOUTS", "DescriptorLayouts already initialized!");
        return;
    }

    // Create Global Descriptor Set Layout (for camera/view data)
    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding = 0;
    cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBinding.descriptorCount = 1;
    cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    cameraBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
    globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    globalLayoutInfo.bindingCount = 1;
    globalLayoutInfo.pBindings = &cameraBinding;

    if (vkCreateDescriptorSetLayout(device, &globalLayoutInfo, nullptr, &s_globalLayout) != VK_SUCCESS) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "Failed to create global descriptor set layout!");
        return;
    }
    LOG_INFO("DESCRIPTOR_LAYOUTS", "Global descriptor set layout created successfully!");

    // Create Material Descriptor Set Layout (for material properties)
    VkDescriptorSetLayoutBinding materialBinding{};
    materialBinding.binding = 0;
    materialBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    materialBinding.descriptorCount = 1;
    materialBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    materialBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
    materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    materialLayoutInfo.bindingCount = 1;
    materialLayoutInfo.pBindings = &materialBinding;

    if (vkCreateDescriptorSetLayout(device, &materialLayoutInfo, nullptr, &s_materialLayout) != VK_SUCCESS) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "Failed to create material descriptor set layout!");
        return;
    }
    LOG_INFO("DESCRIPTOR_LAYOUTS", "Material descriptor set layout created successfully!");

    // Create Texture Descriptor Set Layout (for textures/samplers)
    VkDescriptorSetLayoutBinding textureBinding{};
    textureBinding.binding = 0;
    textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureBinding.descriptorCount = 1;
    textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.bindingCount = 1;
    textureLayoutInfo.pBindings = &textureBinding;

    if (vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &s_textureLayout) != VK_SUCCESS) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "Failed to create texture descriptor set layout!");
        return;
    }
    LOG_INFO("DESCRIPTOR_LAYOUTS", "Texture descriptor set layout created successfully!");

    s_initialized = true;
    LOG_INFO("DESCRIPTOR_LAYOUTS", "All descriptor layouts initialized successfully!");
}

void DescriptorLayouts::cleanup(VkDevice device) {
    if (!s_initialized) {
        return;
    }

    if (s_globalLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, s_globalLayout, nullptr);
        s_globalLayout = VK_NULL_HANDLE;
        LOG_INFO("DESCRIPTOR_LAYOUTS", "Global descriptor layout destroyed");
    }

    if (s_materialLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, s_materialLayout, nullptr);
        s_materialLayout = VK_NULL_HANDLE;
        LOG_INFO("DESCRIPTOR_LAYOUTS", "Material descriptor layout destroyed");
    }

    if (s_textureLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, s_textureLayout, nullptr);
        s_textureLayout = VK_NULL_HANDLE;
        LOG_INFO("DESCRIPTOR_LAYOUTS", "Texture descriptor layout destroyed");
    }

    s_initialized = false;
    LOG_INFO("DESCRIPTOR_LAYOUTS", "All descriptor layouts cleaned up");
}

VkDescriptorSetLayout DescriptorLayouts::getGlobalLayout() {
    if (!s_initialized) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "DescriptorLayouts not initialized! Call init() first.");
        return VK_NULL_HANDLE;
    }
    return s_globalLayout;
}

VkDescriptorSetLayout DescriptorLayouts::getMaterialLayout() {
    if (!s_initialized) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "DescriptorLayouts not initialized! Call init() first.");
        return VK_NULL_HANDLE;
    }
    return s_materialLayout;
}

VkDescriptorSetLayout DescriptorLayouts::getTextureLayout() {
    if (!s_initialized) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "DescriptorLayouts not initialized! Call init() first.");
        return VK_NULL_HANDLE;
    }
    return s_textureLayout;
}

std::vector<VkDescriptorSetLayout> DescriptorLayouts::getAllLayouts() {
    if (!s_initialized) {
        LOG_ERR("DESCRIPTOR_LAYOUTS", "DescriptorLayouts not initialized! Call init() first.");
        return {};
    }
    
    return {
        s_globalLayout,
        s_materialLayout,
        s_textureLayout
    };
}