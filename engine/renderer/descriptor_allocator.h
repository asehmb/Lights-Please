#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class DescriptorAllocator {
public:
    DescriptorAllocator();
    ~DescriptorAllocator();
    void init(VkDevice device, uint8_t poolCount);
    VkDescriptorSet allocate(VkDescriptorSetLayout layout);
    void reset_pools();
private:
    VkDevice m_device;
    uint8_t m_poolCount;
    std::vector<VkDescriptorPool> m_pools;
    VkDescriptorPool create_pool();
    VkDescriptorPool m_currentPool = VK_NULL_HANDLE;
};