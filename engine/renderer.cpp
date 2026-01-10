#include "renderer.h"

#include <SDL.h>
#include <SDL_stdinc.h>
#include <SDL_vulkan.h>
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
			createCommandPool();
			createCommandBuffer();
		}
	}
}

Renderer::~Renderer()
{
	if (commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(device, commandPool, nullptr);
		commandPool = VK_NULL_HANDLE;
	}
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

	LOG_INFO("VULKAN", "Extensions count: {}", static_cast<unsigned>(extensions.size()));
	LOG_INFO("VULKAN", "Extensions:");
	for (const auto& ext : extensions) {
		LOG_INFO("VULKAN", "  {}", ext);
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
	appInfo.apiVersion = VK_API_VERSION_1_0;

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
		LOG_ERR("VULKAN", "vkCreateInstance failed: {}", static_cast<int>(res));
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
				LOG_ERR("VULKAN", "{}", pCallbackData->pMessage);
				return VK_FALSE;
			};

			VkResult r2 = func(instance, &createInfoDbg, nullptr, &debugMessenger);
			if (r2 != VK_SUCCESS) {
				LOG_WARN("VULKAN", "vkCreateDebugUtilsMessengerEXT failed: {}", static_cast<int>(r2));
			} else {
				LOG_INFO("VULKAN", "Debug messenger created");
			}
		} else {
			LOG_WARN("VULKAN", "vkCreateDebugUtilsMessengerEXT not found");
		}
	}

	// Create Vulkan surface from SDL window
	if(!SDL_Vulkan_CreateSurface(window, instance, &surface)){
		LOG_ERR("VULKAN", "Failed to create Vulkan surface from SDL window");
		return false;
	}
	LOG_INFO("RENDERER", "Renderer initialized");

    return instance != VK_NULL_HANDLE;
}

bool Renderer::pickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		LOG_ERR("VULKAN", "Failed to find GPUs with Vulkan support!");
		return false;
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	LOG_INFO("VULKAN", "Found {} Vulkan-capable devices", deviceCount);
	for (const auto& device : devices) {
		// TODO: here you would typically check for device properties and features and 
		// check if they meet your requirements
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		LOG_INFO("VULKAN", "\tDevice Name: {}", deviceProperties.deviceName);
	}

	physicalDevice = devices[0]; // Just pick the first one for now

	if (physicalDevice == VK_NULL_HANDLE) {
		LOG_ERR("VULKAN", "Failed to select a physical device!");
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
		LOG_INFO("VULKAN", "Enabling validation layers for logical device");
		extensions.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);

	}
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);


	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();


	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) == VK_SUCCESS) {
		LOG_INFO("VULKAN", "Logical device created successfully");
	} else {
		LOG_ERR("VULKAN", "Failed to create logical device!");
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
		LOG_ERR("VULKAN", "Swap chain not adequate!");
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
		LOG_ERR("VULKAN", "Failed to create swapchain!");
		return false;
	}
	LOG_INFO("VULKAN", "Swapchain created successfully");
	
	return true;
}

bool Renderer::recreateSwapchain() {
	// Cleanup existing swapchain
	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(device, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE;
	}

	// Recreate the swapchain
	return createSwapchain();
}

bool Renderer::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		LOG_ERR("VULKAN", "Failed to create command pool!");
		return false;
	}

	LOG_INFO("VULKAN", "Created Command Pool succesfully!");

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
		LOG_ERR("VULKAN", "Failed to allocate command buffer!");
		return false;
	}

	LOG_INFO("VULKAN", "Allocated Command Buffer successfully!");

	return true;
}

void Renderer::recordCommandBuffer(uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		LOG_ERR("VULKAN", "Failed to begin cmd buffer recording");
		return;
	}


	pipelineBarrier(commandBuffer, swapchainImages[imageIndex],
		 	VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			0,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
	);

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapchainExtent;



	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// TODO: Set viewports and scissors here

	// TODO: Add drawing commands here
	// vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
	// vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	
	vkCmdEndRenderPass(commandBuffer);


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
	// TODO: where are fences created?
	vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &inFlightFence);

	// Get next image from swapchain
	uint32_t imageIndex;
	// TODO: where is imageAvailableSemaphore created?
	VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
		imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	recordCommandBuffer(imageIndex);
}

// Pipeline Barrier helper function.
// Examples oldLayout/newLayout usage:
// Start Frame -- UNDEFINED, 	COLOR_ATTACHMENT_OPTIMAL -- Clears old junk; ready to paint.
// End Frame -- COLOR_ATTACHMENT_OPTIMAL, PRESENT_SRC_KHR -- Hands the "painting" to the monitor.
// Upload Texture -- TRANSFER_DST_OPTIMAL, SHADER_READ_ONLY_OPTIMAL -- Moved from CPU to GPU; now shaders can see it
// Examples srcAccessMask/dstAccessMask usage:
// Start Frame -- 0, COLOR_ATTACHMENT_WRITE_BIT
// End Frame -- COLOR_ATTACHMENT_WRITE_BIT, MEMORY_READ_BIT
// Upload Texture -- TRANSFER_WRITE_BIT, SHADER_READ_BIT
// Examples srcStageMask/dstStageMask usage:
// Start Frame -- TOP_OF_PIPE_BIT, COLOR_ATTACHMENT_OUTPUT_BIT
// End Frame -- COLOR_ATTACHMENT_OUTPUT_BIT, BOTTOM_OF_PIPE_BIT
// Upload Texture -- TRANSFER_BIT, FRAGMENT_SHADER_BIT
void Renderer::pipelineBarrier(VkCommandBuffer commandBuffer,
						 VkImage image,
						 VkImageLayout oldLayout,
						 VkImageLayout newLayout,
						 VkPipelineStageFlags srcStageMask,
						 VkPipelineStageFlags dstStageMask,
						 VkAccessFlags srcAccessMask,
						 VkAccessFlags dstAccessMask) {
	VkImageMemoryBarrier2 barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
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
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;

	VkDependencyInfo dependencyInfo{};
	dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependencyInfo.imageMemoryBarrierCount = 1;
	dependencyInfo.pImageMemoryBarriers = &barrier;

	vkCmdPipelineBarrier2(
		commandBuffer,
		&dependencyInfo
	);
}