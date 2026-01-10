#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class DescriptorAllocator {
public:
    DescriptorAllocator();
    ~DescriptorAllocator();
    void init(VkDevice device);
    VkDescriptorSet allocate(VkDescriptorSetLayout layout);
    void reset_pools();
private:
    VkDevice m_device;
    std::vector<VkDescriptorPool> m_pools;
    VkDescriptorPool create_pool();
    VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
};