
#include "descriptor_allocator.h"
#include <vulkan/vulkan_core.h>

#define MAX_DESCRIPTOR_SETS 1000

DescriptorAllocator::DescriptorAllocator()
    : m_device(VK_NULL_HANDLE), m_currentPool(VK_NULL_HANDLE) {}


DescriptorAllocator::~DescriptorAllocator() {
    for (auto pool : m_pools) {
        vkDestroyDescriptorPool(m_device, pool, nullptr);
    }
    m_pools.clear();
    m_currentPool = VK_NULL_HANDLE;
}


void DescriptorAllocator::init(VkDevice device) {
    m_device = device;

    // Create the initial pool
    m_currentPool = create_pool();
    m_pools.push_back(m_currentPool);
}

VkDescriptorPool DescriptorAllocator::create_pool() {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = MAX_DESCRIPTOR_SETS;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_DESCRIPTOR_SETS;

    VkDescriptorPool descriptorPool;
    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }

    return descriptorPool;
}

VkDescriptorSet DescriptorAllocator::allocate(VkDescriptorSetLayout layout) {
    if (m_currentPool == VK_NULL_HANDLE) {
        m_currentPool = create_pool();
        m_pools.push_back(m_currentPool);
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_currentPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet;
    VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet);
    if (result == VK_SUCCESS) {
        return descriptorSet;
    } else if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY) {
        // Pool is full, create a new one
        m_currentPool = create_pool();
        m_pools.push_back(m_currentPool);

        allocInfo.descriptorPool = m_currentPool;
        if (vkAllocateDescriptorSets(m_device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate descriptor set from new pool!");
        }
        return descriptorSet;
    } else {
        throw std::runtime_error("Failed to allocate descriptor set!");
    }
}

void DescriptorAllocator::reset_pools() {
    for (auto pool : m_pools) {
        vkResetDescriptorPool(m_device, pool, 0);
    }
    m_currentPool = m_pools[0];
}

