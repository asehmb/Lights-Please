#include "renderer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_beta.h>
#include <vector>
#include "logger.h"
#include "pipeline.h"
#include <set>
#include <cstring>
#include "pipeline.h"

Renderer::Renderer(SDL_Window* window)
	: instance(VK_NULL_HANDLE), surface(VK_NULL_HANDLE)
{
	if (!initialize(window)) {
		LOG_ERR("RENDERER", "Renderer initialization failed");
	}
	// Pick physical device and create logical device if initialization succeeded
	if (instance != VK_NULL_HANDLE && surface != VK_NULL_HANDLE) {
		if (pickPhysicalDevice()) {
			initLogicalDevice();
			createSwapchain();
			createSwapchainImageViews();
			createCommandPool();
			createCommandBuffer();
			createSemaphores();
			createFences();
			createDescriptorSetLayouts();
			createRenderPass();
			createFramebuffers();
			
		}
	}
}

Renderer::~Renderer()
{
	vkDeviceWaitIdle(device);
	if (!framebuffers.empty()) {
		for (auto framebuffer : framebuffers) {
			if (framebuffer != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(device, framebuffer, nullptr);
			}
		}
		framebuffers.clear();
	}
	if (renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(device, renderPass, nullptr);
		renderPass = VK_NULL_HANDLE;
	}
	if (globalDescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, globalDescriptorSetLayout, nullptr);
		globalDescriptorSetLayout = VK_NULL_HANDLE;
	}
	if (materialDescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, materialDescriptorSetLayout, nullptr);
		materialDescriptorSetLayout = VK_NULL_HANDLE;
	}
	if (inFlightFence != VK_NULL_HANDLE) {
		vkDestroyFence(device, inFlightFence, nullptr);
		inFlightFence = VK_NULL_HANDLE;
	}
	if (renderFinishedSemaphore != VK_NULL_HANDLE) {
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		renderFinishedSemaphore = VK_NULL_HANDLE;
	}
	if (imageAvailableSemaphore != VK_NULL_HANDLE) {
		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		imageAvailableSemaphore = VK_NULL_HANDLE;
	}
	if (commandBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
		commandBuffer = VK_NULL_HANDLE;
	}
	if (commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(device, commandPool, nullptr);
		commandPool = VK_NULL_HANDLE;
	}
	if (!swapchainImageViews.empty()) {
		for (auto imageView : swapchainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		swapchainImageViews.clear();
	}
	// Clear layout tracking
	swapchainImageLayouts.clear();
	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE;
	}
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
	if (device != VK_NULL_HANDLE) {
		vkDestroyDevice(device, nullptr);
		device = VK_NULL_HANDLE;
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

	// needed for macos/MoltenVK
	extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

#ifndef NDEBUG
    // Add Debug Utils if in Debug mode
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	LOG_INFO("RENDERER", "Extensions count: {}", static_cast<unsigned>(extensions.size()));
	LOG_INFO("RENDERER", "Extensions:");
	for (const auto& ext : extensions) {
		LOG_INFO("RENDERER", "\t{}", ext);
	}

	// Determine whether to enable Validation layers (only in debug builds)
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
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	}


	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Lights Please";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;

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
		LOG_ERR("RENDERER", "vkCreateInstance failed: {}", static_cast<int>(res));
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
				LOG_ERR("RENDERER", "{}", pCallbackData->pMessage);
				return VK_FALSE;
			};

			VkResult r2 = func(instance, &createInfoDbg, nullptr, &debugMessenger);
			if (r2 != VK_SUCCESS) {
				LOG_WARN("RENDERER", "vkCreateDebugUtilsMessengerEXT failed: {}", static_cast<int>(r2));
			} else {
				LOG_INFO("RENDERER", "Debug messenger created");
			}
		} else {
			LOG_WARN("RENDERER", "vkCreateDebugUtilsMessengerEXT not found");
		}
	}

	// Create Vulkan surface from SDL window
	if(!SDL_Vulkan_CreateSurface(window, instance, &surface)){
		LOG_ERR("RENDERER", "Failed to create Vulkan surface from SDL window");
		return false;
	}
	LOG_INFO("RENDERER", "Renderer initialized");

    return instance != VK_NULL_HANDLE;
}

bool Renderer::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		LOG_ERR("PHYSICAL_DEVICE", "Failed to find GPUs with Vulkan support!");
		return false;
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	LOG_INFO("PHYSICAL_DEVICE", "Found {} Vulkan-capable devices", deviceCount);
	for (const auto& device : devices) {
		// TODO: here you would typically check for device properties and features and 
		// check if they meet your requirements
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		LOG_INFO("PHYSICAL_DEVICE", "\tDevice Name: {}", deviceProperties.deviceName);
	}

	physicalDevice = devices[0]; // Just pick the first one for now

	if (physicalDevice == VK_NULL_HANDLE) {
		LOG_ERR("PHYSICAL_DEVICE", "Failed to select a physical device!");
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


	VkDeviceCreateInfo createInfo{};

	std::vector<const char*> extensions;
	if (enableValidationLayers && !validationLayers.empty()) {
		LOG_INFO("LOGICAL_DEVICE", "Enabling validation layers for logical device");
		extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);

	}
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	extensions.push_back(VK_KHR_MULTIVIEW_EXTENSION_NAME);
	extensions.push_back(VK_KHR_MAINTENANCE_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
	extensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);

	VkPhysicalDeviceSynchronization2Features sync2Features = {};
	sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	sync2Features.pNext = nullptr;
	sync2Features.synchronization2 = VK_TRUE;
	createInfo.pNext = &sync2Features;


	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();


	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) == VK_SUCCESS) {

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
		LOG_INFO("LOGICAL_DEVICE", "Logical device created successfully");
	} else {
		LOG_ERR("LOGICAL_DEVICE", "Failed to create logical device!");
	}
}

VkPresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	// Prefer MAILBOX if available, otherwise use FIFO which is guaranteed to be available
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			LOG_INFO("VULKAN", "Selected VK_PRESENT_MODE_MAILBOX_KHR for swapchain");
			return availablePresentMode;
		}
	}
	LOG_INFO("VULKAN", "Selected VK_PRESENT_MODE_FIFO_KHR for swapchain");
	return VK_PRESENT_MODE_FIFO_KHR;
}
VkSurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	// Prefer SRGB format with UNORM color space
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	// Otherwise return the first available format
	return availableFormats[0];
}
VkExtent2D Renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	// If currentExtent is not set to the special value, return it
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		// Otherwise, we can set the extent to what we want (clamped to allowed extents)
		VkExtent2D actualExtent = {800, 600}; // Placeholder for desired width/height

		actualExtent.width = std::max(capabilities.minImageExtent.width,
			std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height,
			std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}


bool Renderer::createSwapchain() {
	SwapChainSupportDetails surfaceCapabilities = querySwapChainSupport(physicalDevice, surface);
	bool swapChainAdequate = !surfaceCapabilities.formats.empty() && !surfaceCapabilities.presentModes.empty();
	if (!swapChainAdequate) {
		LOG_ERR("SWAPCHAIN", "Swap chain not adequate!");
		return false;
	}

	int imageCount = surfaceCapabilities.capabilities.minImageCount + 1;
	if (surfaceCapabilities.capabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.capabilities.maxImageCount) {
		imageCount = surfaceCapabilities.capabilities.maxImageCount;
	}
	

	std::vector<VkPresentModeKHR> availablePresentModes;
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	availablePresentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, availablePresentModes.data());

	std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	availableSurfaceFormats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, availableSurfaceFormats.data());

	
	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.preTransform = surfaceCapabilities.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.imageFormat = chooseSwapSurfaceFormat(availableSurfaceFormats).format;
	swapchainCreateInfo.imageColorSpace = chooseSwapSurfaceFormat(availableSurfaceFormats).colorSpace;
	swapchainCreateInfo.presentMode = chooseSwapPresentMode(availablePresentModes);
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageExtent = chooseSwapExtent(surfaceCapabilities.capabilities);
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainExtent = swapchainCreateInfo.imageExtent;

	if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
		LOG_ERR("SWAPCHAIN", "Failed to create swapchain!");
		return false;
	}
	LOG_INFO("SWAPCHAIN", "Swapchain created successfully");
	
	// Retrieve swapchain images
	uint32_t swapchainImageCount = 0;
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr);
	swapchainImages.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data());
	swapchainImageFormat = swapchainCreateInfo.imageFormat;
	
	// Initialize layout tracking - swapchain images start in undefined layout
	swapchainImageLayouts.resize(swapchainImageCount, VK_IMAGE_LAYOUT_UNDEFINED);
	
	return true;
}

bool Renderer::recreateSwapchain() {
	vkDeviceWaitIdle(device);


	// Cleanup existing swapchain

	if (!framebuffers.empty()) {
		for (auto framebuffer : framebuffers) {
			if (framebuffer != VK_NULL_HANDLE) {
				vkDestroyFramebuffer(device, framebuffer, nullptr);
			}
		}
		framebuffers.clear();
	}
	if (!swapchainImageViews.empty()) {
		for (auto imageView : swapchainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		swapchainImageViews.clear();
	}
	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE;
	}
	// Clear layout tracking for old swapchain
	swapchainImageLayouts.clear();

	// Recreate the swapchain
	createSwapchain();
	createSwapchainImageViews();
	createFramebuffers();
	return true;
}

bool Renderer::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		LOG_ERR("COMMAND_POOL", "Failed to create command pool!");
		return false;
	}

	LOG_INFO("COMMAND_POOL", "Created Command Pool succesfully!");
	return true;
}

bool Renderer::createCommandBuffer() {
	// Allocate a single command buffer from the command pool
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	// maybe VK_COMMAND_BUFFER_LEVEL_SECONDARY for secondary buffers
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
		LOG_ERR("COMMAND_BUFFER", "Failed to allocate command buffer!");
		return false;
	}

	LOG_INFO("COMMAND_BUFFER", "Allocated Command Buffer successfully!");
	return true;
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		LOG_ERR("RECORD_COMMAND_BUFFER", "Failed to begin cmd buffer recording");
		return;
	}

	// Get the current layout of this swapchain image
	VkImageLayout currentLayout = swapchainImageLayouts[imageIndex];

	transitionImage(commandBuffer, swapchainImages[imageIndex],
		 	VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
	);

	// Update layout tracking
	swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;
	renderPassInfo.framebuffer = framebuffers[imageIndex];
	renderPassInfo.clearValueCount = 1;
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.pClearValues = &clearColor;



	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	GraphicPipeline* currentPipeline = nullptr;

	// TODO: Set viewports and scissors here
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapchainExtent.width);
	viewport.height = static_cast<float>(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	for (const auto& drawable : drawables) {
		if (currentPipeline != drawable.material->pipeline.get()) {
			currentPipeline = drawable.material->pipeline.get();
			currentPipeline->bind(commandBuffer);
		}
		// Bind descriptor sets, vertex buffers, index buffers, etc. here as needed
		// drawable.mesh->bind(commandBuffer);
		// vkCmdDrawIndexed(commandBuffer, drawable.mesh->getIndexCount(), 1, 0, 0, 0);
		drawMesh(commandBuffer, drawable.mesh);
	}
	
	vkCmdEndRenderPass(commandBuffer);

	transitionImage(commandBuffer, swapchainImages[imageIndex],
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_2_MEMORY_READ_BIT
	);

	// Update layout tracking
	swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	vkEndCommandBuffer(commandBuffer);
}

std::unique_ptr<GraphicPipeline> Renderer::createOpaquePipeline(const char* vertexShaderPath, const char* fragmentShaderPath) {
	PipelineBuilder builder;

	builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
	builder.set_cull_mode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
	builder.enable_depthtest(true, VK_COMPARE_OP_LESS);
	builder.disable_blending();
	// TODO: Set proper pipeline layout instead of VK_NULL_HANDLE
	builder.pipelineLayout = VK_NULL_HANDLE;

	try {
		GraphicPipeline pipeline = builder.build(device, renderPass, vertexShaderPath, fragmentShaderPath);
		return std::make_unique<GraphicPipeline>(std::move(pipeline));
	} catch (const std::exception& e) {
		LOG_ERR("PIPELINE", "Failed to create opaque pipeline: {}", e.what());
		return nullptr;
	}
}



void Renderer::drawFrame() {
	
	// wait for previous frame
	vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);

	// Get next image from swapchain
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
		imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapchain();
		return;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		LOG_ERR("DRAW_FRAME", "Failed to acquire swap chain image!");
		return;
	}
	vkResetFences(device, 1, &inFlightFence);


	recordCommandBuffer(imageIndex);

	// Submit command buffer
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphore }; 
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
		LOG_ERR("DRAW_FRAME", "Failed to submit draw command buffer!");
		return;
	}

	// Present the image
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapchain();
	} else if (result != VK_SUCCESS) {
		LOG_ERR("DRAW_FRAME", "Failed to present swap chain image!");
	}
	vkQueueWaitIdle(presentQueue);

}

// Pipeline Barrier helper function.
// Examples oldLayout/newLayout usage:
// Start Frame -- UNDEFINED, 	COLOR_ATTACHMENT_OPTIMAL -- Clears old junk; ready to paint.
// End Frame -- COLOR_ATTACHMENT_OPTIMAL, PRESENT_SRC_KHR -- Hands the "painting" to the monitor.
// Upload Texture -- TRANSFER_DST_OPTIMAL, SHADER_READ_ONLY_OPTIMAL -- Moved from CPU to GPU; now shaders can see it.
// Examples srcAccessMask/dstAccessMask usage:
// Start Frame -- 0, COLOR_ATTACHMENT_WRITE_BIT.
// End Frame -- COLOR_ATTACHMENT_WRITE_BIT, MEMORY_READ_BIT.
// Upload Texture -- TRANSFER_WRITE_BIT, SHADER_READ_BIT.
// Examples srcStageMask/dstStageMask usage:
// Start Frame -- TOP_OF_PIPE_BIT, COLOR_ATTACHMENT_OUTPUT_BIT.
// End Frame -- COLOR_ATTACHMENT_OUTPUT_BIT, BOTTOM_OF_PIPE_BIT.
// Upload Texture -- TRANSFER_BIT, FRAGMENT_SHADER_BIT.
void Renderer::transitionImage(VkCommandBuffer commandBuffer,
						 VkImage image,
						 VkImageLayout oldLayout,
						 VkImageLayout newLayout,
						 VkPipelineStageFlags2 srcStageMask,
						 VkPipelineStageFlags2 dstStageMask,
						 VkAccessFlags2 srcAccessMask,
						 VkAccessFlags2 dstAccessMask) {
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.srcStageMask = srcStageMask;
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstStageMask = dstStageMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;
	dependencyInfo.memoryBarrierCount = 0;
	dependencyInfo.pMemoryBarriers = nullptr;
	dependencyInfo.bufferMemoryBarrierCount = 0;
	dependencyInfo.pBufferMemoryBarriers = nullptr;

	PFN_vkCmdPipelineBarrier2KHR fp_vkCmdPipelineBarrier2 = nullptr;
	fp_vkCmdPipelineBarrier2 =
		(PFN_vkCmdPipelineBarrier2KHR)
		vkGetDeviceProcAddr(device, "vkCmdPipelineBarrier2KHR");

	fp_vkCmdPipelineBarrier2(
		commandBuffer,
		&dependencyInfo
	);
}

void Renderer::drawMesh(VkCommandBuffer commandBuffer, Mesh* mesh) {

	// bind vertex and index buffers
	mesh->bind(commandBuffer);
	
	// draw call
	mesh->draw(commandBuffer);

}

void Renderer::createSemaphores() {
	// Create semaphores for image availability and render finished
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
		LOG_ERR("SEMAPHORE", "Failed to create semaphores!");
	} else {
		LOG_INFO("SEMAPHORE", "Semaphores created successfully!");
	}
}

void Renderer::createFences() {
	// Create fence for in-flight frame
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so we don't wait on first frame

	if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
		LOG_ERR("FENCE", "Failed to create fence!");
	} else {
		LOG_INFO("FENCE", "Fence created successfully!");
	}
}


bool Renderer::createRenderPass() {

	LOG_INFO("RENDER_PASS", "Creating color attachment");

	VkAttachmentDescription2 colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	colorAttachment.format = swapchainImageFormat; // TODO: needs to be created
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // keep the rendered contents
	// TODO: we don't care about stencil for now, when should we?
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	

	// subpass(es) and references
	VkAttachmentReference2 colorAttachmentRef{};
	colorAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	colorAttachmentRef.attachment = 0; // index in the attachment descriptions array
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	LOG_INFO("RENDER_PASS", "Creating subpass");


	VkSubpassDescription2 subpass{};
	subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	LOG_INFO("RENDER_PASS", "Creating render pass");
	
	VkRenderPassCreateInfo2 renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	renderPassInfo.pDependencies = nullptr;
	renderPassInfo.pNext = nullptr;

	LOG_INFO("RENDER_PASS", "Render pass info created, creating render pass now");

	if (vkGetDeviceProcAddr(device, "vkCreateRenderPass2KHR") == nullptr) {
		LOG_ERR("RENDER_PASS", "vkCreateRenderPass2 function not found!");
		return false;
	}

	// MoltenVK things here
	PFN_vkCreateRenderPass2KHR fpCreateRenderPass2KHR = nullptr;
	fpCreateRenderPass2KHR =
		(PFN_vkCreateRenderPass2KHR)
		vkGetDeviceProcAddr(device, "vkCreateRenderPass2KHR");


	if (fpCreateRenderPass2KHR(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		LOG_ERR("RENDER_PASS", "Failed to create render pass!");
		return false;
	}

	LOG_INFO("RENDER_PASS", "Render pass created successfully");

	return true;
}


void Renderer::createDescriptorSetLayouts() {
	// Global Descriptor Set Layout
	VkDescriptorSetLayoutBinding globalBinding{};
	globalBinding.binding = 0;
	globalBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	globalBinding.descriptorCount = 1;
	globalBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	globalBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo globalLayoutInfo{};
	globalLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	globalLayoutInfo.bindingCount = 1;
	globalLayoutInfo.pBindings = &globalBinding;

	if (vkCreateDescriptorSetLayout(device, &globalLayoutInfo, nullptr, &globalDescriptorSetLayout) != VK_SUCCESS) {
		LOG_ERR("DESCRIPTOR_SET_LAYOUT", "Failed to create global descriptor set layout!");
	} else {
		LOG_INFO("DESCRIPTOR_SET_LAYOUT", "Global descriptor set layout created successfully!");
	}

	// Material Descriptor Set Layout
	VkDescriptorSetLayoutBinding materialBinding{};
	materialBinding.binding = 0;
	materialBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	materialBinding.descriptorCount = 1;
	materialBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	materialBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
	materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	materialLayoutInfo.bindingCount = 1;
	materialLayoutInfo.pBindings = &materialBinding;

	if (vkCreateDescriptorSetLayout(device, &materialLayoutInfo, nullptr, &materialDescriptorSetLayout) != VK_SUCCESS) {
		LOG_ERR("DESCRIPTOR_SET_LAYOUT", "Failed to create material descriptor set layout!");
	} else {
		LOG_INFO("DESCRIPTOR_SET_LAYOUT", "Material descriptor set layout created successfully!");
	}
}


bool Renderer::createSwapchainImageViews() {
	swapchainImageViews.resize(swapchainImages.size());	

	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapchainImages[i];
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = swapchainImageFormat;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
			LOG_ERR("IMAGE_VIEW", "Failed to create image views!");
			return false;
		}
	}

	LOG_INFO("IMAGE_VIEW", "Created {} swapchain image views successfully", swapchainImageViews.size());

	return true;
}

bool Renderer::createFramebuffers() {
	framebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); i++) {
		VkImageView attachments[] = {
			swapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
			LOG_ERR("FRAMEBUFFER", "Failed to create framebuffer!");
			return false;
		}
	}

	LOG_INFO("FRAMEBUFFER", "Framebuffers created successfully");

	return true;
}