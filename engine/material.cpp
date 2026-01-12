#include "material.hpp"
#include "logger.h"
#include "vulkan/vulkan_core.h"

Material::Material(VkDevice device, VmaAllocator allocator, DescriptorLayouts& descriptorLayouts) 
    : m_device(device), m_allocator(allocator), 
      m_diffuseTexture(VK_NULL_HANDLE),
      m_specularTexture(VK_NULL_HANDLE),
      m_normalTexture(VK_NULL_HANDLE) {

    createPipelineLayout(descriptorLayouts);
}

Material::~Material() {
    // Cleanup if needed
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
}

void Material::createPipelineLayout(DescriptorLayouts& descriptorLayouts) {
    // Create pipeline layout here if needed
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    std::vector<VkDescriptorSetLayout> layouts = descriptorLayouts.getAllLayouts();
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }
    LOG_INFO("MATERIAL", "Created Pipeline Layout");
    
}

void Material::setDiffuseTexture(VkImageView textureView) {
    m_diffuseTexture = textureView;
}

void Material::setSpecularTexture(VkImageView textureView) {
    m_specularTexture = textureView;
}

void Material::setNormalTexture(VkImageView textureView) {
    m_normalTexture = textureView;
}