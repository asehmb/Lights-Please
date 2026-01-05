#pragma once

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>

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
    bool pickPhysicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void initLogicalDevice();

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
    // Add more Vulkan objects as needed (e.g., device, swapchain, etc.)
};