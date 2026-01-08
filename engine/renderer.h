#pragma once

#include <SDL.h>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>

class Renderer {
public:
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };
    Renderer(struct SDL_Window* window);
    ~Renderer();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // Initialize the Vulkan instance and create an SDL Vulkan surface.
    // Returns true on success.
    bool initialize(SDL_Window* window);

private:
    VkInstance instance{VK_NULL_HANDLE};
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    std::vector<const char*> instanceExtensions;
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    std::vector<const char*> validationLayers;
    VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
    bool enableValidationLayers{false};
    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkExtent2D swapchainExtent{};
    void initLogicalDevice();
    bool createSwapchain();
    bool pickPhysicalDevice();
    bool recreateSwapchain();

    bool createCommandPool();
    bool createCommandBuffer();
    void recordCommandBuffer(uint32_t imageIndex);

    bool createGraphicsPipeline();
};