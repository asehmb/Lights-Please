// engine/descriptor_layouts.h
#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class DescriptorLayouts {
public:
    static void init(VkDevice device);
    static void cleanup(VkDevice device);
    
    // Global layouts (shared across all materials)
    static VkDescriptorSetLayout getGlobalLayout();      // Camera, lights, etc.
    static VkDescriptorSetLayout getMaterialLayout();    // Material properties
    static VkDescriptorSetLayout getTextureLayout();     // Textures/samplers
    
    // Get all layouts for pipeline creation
    static std::vector<VkDescriptorSetLayout> getAllLayouts();
    
private:
    static VkDescriptorSetLayout s_globalLayout;
    static VkDescriptorSetLayout s_materialLayout;
    static VkDescriptorSetLayout s_textureLayout;
    static bool s_initialized;
};