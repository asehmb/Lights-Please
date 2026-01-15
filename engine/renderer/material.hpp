#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "pipeline.h"
#ifndef VMA_INCLUDE_ONLY
#define VMA_INCLUDE_ONLY
#endif
#include "../external/vk_mem_alloc.h"
#include "descriptor_layout.h"
#include "ubo.h"

class Material {
public:
    struct PipelineConfig {
        VkPipelineViewportStateCreateInfo viewportInfo;
        VkPipelineInputAssemblyStateCreateInfo inputAssembly;
        VkPipelineRasterizationStateCreateInfo rasterizer;
        VkPipelineMultisampleStateCreateInfo multisampling;
        VkPipelineColorBlendAttachmentState colorBlendAttachment;
        VkPipelineDepthStencilStateCreateInfo depthStencil;
        VkPipelineLayout pipelineLayout;
        VkRenderPass renderPass;
        uint32_t subpass = 0;
    };
    Material(VkDevice device, VmaAllocator allocator, DescriptorLayouts& descriptorLayouts);
    ~Material();

    void initializeDescriptorSets(class DescriptorAllocator* allocator);
    void updateMaterialUBO();

    VkPipelineLayout pipelineLayout;
    std::shared_ptr<GraphicPipeline> pipeline;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    
    // Material-specific descriptor sets
    VkDescriptorSet materialDescriptorSet{VK_NULL_HANDLE};
    VkDescriptorSet textureDescriptorSet{VK_NULL_HANDLE};


    // Setters
    void setDiffuseTexture(VkImageView textureView);
    void setSpecularTexture(VkImageView textureView);
    void setNormalTexture(VkImageView textureView);
    void updateTextureDescriptors(VkSampler sampler);
    void setDefaultSampler(VkSampler sampler) { m_defaultSampler = sampler; }

    // Getters
    VkImageView getDiffuseTexture() const { return m_diffuseTexture; }
    VkImageView getSpecularTexture() const { return m_specularTexture; }
    VkImageView getNormalTexture() const { return m_normalTexture; }
    VkSampler getDefaultSampler() const { return m_defaultSampler; }
    void createPipelineLayout(DescriptorLayouts& descriptorLayouts);
    
private:
    VkDevice m_device;
    VmaAllocator m_allocator;
    VkImageView m_diffuseTexture;
    VkImageView m_specularTexture;
    VkSampler m_defaultSampler;
    VkImageView m_normalTexture;
    UBO materialUBO;


};