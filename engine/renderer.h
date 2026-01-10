#pragma once

#include <SDL.h>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <memory>
#include "pipeline.h"

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
    
    // Public rendering methods
    void beginFrame();
    void endFrame();
    void drawTriangle();

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
    
    // Swapchain framebuffers and synchronization
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> framebuffers;
    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore{VK_NULL_HANDLE};
    VkFence inFlightFence{VK_NULL_HANDLE};
    uint32_t currentFrame = 0;

    VkRenderingAttachmentInfo colorAttachmentInfo{};
    
    std::unique_ptr<GraphicPipeline> meshPipeline;
    
    
    void initLogicalDevice();
    bool createSwapchain();
    bool pickPhysicalDevice();
    bool recreateSwapchain();

    bool createCommandPool();
    bool createCommandBuffer();
    void recordCommandBuffer(uint32_t imageIndex);
    
    bool createSwapchainImageViews();
    bool createFramebuffers();
    bool createSyncObjects();

    void drawFrame();

    void pipelineBarrier(VkCommandBuffer commandBuffer,
                         VkImage image,
                         VkImageLayout oldLayout,
                         VkImageLayout newLayout,
                         VkPipelineStageFlags srcStageMask,
                         VkPipelineStageFlags dstStageMask,
                         VkAccessFlags srcAccessMask,
                         VkAccessFlags dstAccessMask);

    std::unique_ptr<GraphicPipeline> createOpaquePipeline(const char* vertexShaderPath, const char* fragmentShaderPath);
};