#pragma once

#include <SDL.h>
#include <vulkan/vulkan.h>
#include <vector>

class Renderer {
public:
    Renderer(struct SDL_Window* window);
    ~Renderer();

    // Initialize the Vulkan instance and create an SDL Vulkan surface.
    // Returns true on success.
    bool initialize(SDL_Window* window);

private:
    VkInstance instance{VK_NULL_HANDLE};
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    std::vector<const char*> instanceExtensions;
    // Add more Vulkan objects as needed (e.g., device, swapchain, etc.)
};