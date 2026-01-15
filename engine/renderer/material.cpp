#include "material.hpp"
#include "../logger.h"
#include "vulkan/vulkan_core.h"
#include "descriptor_allocator.h"

Material::Material(VkDevice device, VmaAllocator allocator, DescriptorLayouts& descriptorLayouts) 
    : m_device(device), m_allocator(allocator), 
      m_diffuseTexture(VK_NULL_HANDLE),
      m_specularTexture(VK_NULL_HANDLE),
      m_normalTexture(VK_NULL_HANDLE) {

    createPipelineLayout(descriptorLayouts);
    materialUBO.create(device, allocator);
}

Material::~Material() {
    // Cleanup descriptor sets are handled by DescriptorAllocator
    // VkDescriptorSets don't need individual cleanup when using DescriptorAllocator
    
    // Cleanup UBO and pipeline layout
    materialUBO.cleanup(m_allocator);
    if (pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
    
    LOG_INFO("MATERIAL", "Material destroyed");
}

void Material::createPipelineLayout(DescriptorLayouts& descriptorLayouts) {
    // Create pipeline layout with all descriptor set layouts
    std::vector<VkDescriptorSetLayout> layouts = descriptorLayouts.getAllLayouts();
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

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

void Material::initializeDescriptorSets(DescriptorAllocator* allocator) {
    // Allocate material descriptor set
    materialDescriptorSet = allocator->allocate(DescriptorLayouts::getMaterialLayout());
    LOG_INFO("MATERIAL", "Allocated material descriptor set");
    
    // Allocate texture descriptor set
    textureDescriptorSet = allocator->allocate(DescriptorLayouts::getTextureLayout());
    LOG_INFO("MATERIAL", "Allocated texture descriptor set");
    
    // Update material UBO descriptor
    updateMaterialUBO();


    if (m_defaultSampler != VK_NULL_HANDLE) {
        updateTextureDescriptors(m_defaultSampler);
    }
}

void Material::updateMaterialUBO() {
    if (materialDescriptorSet == VK_NULL_HANDLE) {
        LOG_WARN("MATERIAL", "No descriptor set allocated");
        return; // No descriptor set allocated yet
    }
    
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = materialUBO.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GlobalUniforms); // TODO: Should be material-specific data size
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = materialDescriptorSet;
    descriptorWrite.dstBinding = 0; // Assuming material UBO is binding 0 in material layout
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    
    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    LOG_INFO("MATERIAL", "Updated material UBO descriptor");
}

void Material::updateTextureDescriptors(VkSampler sampler) {
    if (textureDescriptorSet == VK_NULL_HANDLE) {
        LOG_WARN("MATERIAL", "Attempted to update texture, but Descriptor Set is NULL!");
        return;
    }
    if (m_diffuseTexture == VK_NULL_HANDLE) {
        LOG_WARN("MATERIAL", "Attempted to update texture, but Diffuse Texture View is NULL!");
        return;
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_diffuseTexture;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = textureDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
}