#include "renderer.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "logger.h"

Renderer::Renderer(SDL_Window* window)
	: instance(VK_NULL_HANDLE), surface(VK_NULL_HANDLE)
{
	if (!initialize(window)) {
		LOG_ERR("Renderer initialization failed");
	}
}

Renderer::~Renderer()
{
	if (surface != VK_NULL_HANDLE && instance != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		surface = VK_NULL_HANDLE;
	}

	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}
}

bool Renderer::initialize(SDL_Window* window)
{
	// Get the number of extensions SDL requires
    uint32_t extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);

    // Allocate a vector and have SDL fill it
    std::vector<const char*> extensions(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensions.data());

    //  Vulkan SDKs on macOS require it.
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    
    // (Optional) Add Debug Utils if you are in Debug mode
    // extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	LOG_INFO("Extensions count: {}", static_cast<unsigned>(extensions.size()));
	LOG_INFO("Extensions:");
	for (const auto& ext : extensions) {
		LOG_INFO("  {}", ext);
	}

    //fill out the Instance Info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    //Set the Portability Flag for macOS/MoltenVK
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    // Create the instance
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        LOG_ERR("Failed to create Vulkan instance!");
		return false;
    }
    return instance != VK_NULL_HANDLE;
}