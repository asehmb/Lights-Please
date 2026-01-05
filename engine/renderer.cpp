#include "renderer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "logger.h"
#include <set>
#include <cstring>

Renderer::Renderer(SDL_Window* window)
	: instance(VK_NULL_HANDLE), surface(VK_NULL_HANDLE)
{
	if (!initialize(window)) {
		LOG_ERR("Renderer initialization failed");
	}
	// Pick physical device and create logical device if initialization succeeded
	if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
		if (pickPhysicalDevice()) {
			initLogicalDevice();
		}
	}
}

Renderer::~Renderer()
{
	if (surface != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		surface = VK_NULL_HANDLE;
	}
	if (debugMessenger != VK_NULL_HANDLE) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, nullptr);
			debugMessenger = VK_NULL_HANDLE;
		}
	}
	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}
	if (device != VK_NULL_HANDLE) {
		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
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

	//  Vulkan SDKs on macOS may require the portability enumeration extension.
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    
    // (Optional) Add Debug Utils if you are in Debug mode
    // extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	LOG_INFO("Extensions count: {}", static_cast<unsigned>(extensions.size()));
	LOG_INFO("Extensions:");
	for (const auto& ext : extensions) {
		LOG_INFO("  {}", ext);
	}

	// Determine whether to enable validation layers (only in debug builds)
#if LIGHTS_PLEASE_LOG_ENABLED
	enableValidationLayers = true;
#else
	enableValidationLayers = false;
#endif

	// If validation layers are enabled, request the standard validation layer
	if (enableValidationLayers) {
		validationLayers = { "VK_LAYER_KHRONOS_validation" };
		// ensure debug utils extension is requested
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// fill out the Instance Info
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

	// layers
	if (enableValidationLayers && !validationLayers.empty()) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

    //Set the Portability Flag for macOS/MoltenVK
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	// Create the instance
	VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);
	if (res != VK_SUCCESS) {
		LOG_ERR("vkCreateInstance failed: {}", static_cast<int>(res));
		return false;
	}

	// If validation is enabled, set up the debug messenger
	if (enableValidationLayers) {
		// Create debug messenger via extension function
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			VkDebugUtilsMessengerCreateInfoEXT createInfoDbg{};
			createInfoDbg.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfoDbg.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
											VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfoDbg.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfoDbg.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
											  VkDebugUtilsMessageTypeFlagsEXT messageTypes,
											  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
											  void* pUserData) -> VkBool32 {
				(void)messageSeverity; (void)messageTypes; (void)pUserData;
				LOG_ERR("[VULKAN] {}", pCallbackData->pMessage);
				return VK_FALSE;
			};

			VkResult r2 = func(instance, &createInfoDbg, nullptr, &debugMessenger);
			if (r2 != VK_SUCCESS) {
				LOG_WARN("vkCreateDebugUtilsMessengerEXT failed: {}", static_cast<int>(r2));
			} else {
				LOG_INFO("Debug messenger created");
			}
		} else {
			LOG_WARN("vkCreateDebugUtilsMessengerEXT not found");
		}
	}
    return instance != VK_NULL_HANDLE;
}

bool Renderer::pickPhysicalDevice() {
	// Placeholder for physical device selection logic
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		LOG_ERR("Failed to find GPUs with Vulkan support!");
		return false;
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	LOG_INFO("Found {} Vulkan-capable devices", deviceCount);
	for (const auto& device : devices) {
		// TODO: here you would typically check for device properties and features
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		LOG_INFO("Device Name: {}", deviceProperties.deviceName);
	}

	physicalDevice = devices[0]; // Just pick the first one for now

	if (physicalDevice == VK_NULL_HANDLE) {
		LOG_ERR("Failed to select a physical device!");
		return false;
	}
	
	return true;
}

Renderer::QueueFamilyIndices Renderer::findQueueFamilies(VkPhysicalDevice device) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}

	return indices;
}	

void Renderer::initLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		LOG_ERR("Failed to create logical device!");
	}
}