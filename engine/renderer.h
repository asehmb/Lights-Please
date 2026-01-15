#pragma once


#include <SDL.h>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <memory>
#include "camera.h"
#include "external/vk_mem_alloc.h"
#include "material.hpp"
#include "mesh.h"
// #include "pipeline.h"
#include "descriptor_allocator.h"
#include "descriptor_layout.h"
#include "ubo.h"
#include "texture.h"

#define MAX_FRAMES_IN_FLIGHT 3 // max is 3 because min is set to 2 and max is min+1 or less

class Renderer {
public:
    struct Drawable { // small struct to hold a mesh and its material
        Mesh* mesh;
        Material* material;
    };

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

    void addDrawable(const Drawable& drawable) {
        drawables.push_back(drawable);
    }

    Drawable& getDrawable(size_t index) {
        return drawables[index];
    }

    Drawable createDrawable(Mesh* mesh, Material* material) {
        Drawable drawable{mesh, material};
        drawables.push_back(drawable);
        return drawable;
    }
    
    void createTriangleDrawable();
    void drawFrame();
    VkDevice getVulkanDevice() const { return device; }
    VmaAllocator getVmaAllocator() const { return vmaAllocator; }
    DescriptorLayouts& getDescriptorLayouts() { return *descriptorLayouts; }
    VkExtent2D getSwapChainExtent() const { return swapchainExtent; }
    VkRenderPass getRenderPass() const { return renderPass; }
    VkCommandPool getCommandPool() const { return commandPool; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }
    VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }

    void setCamera(std::shared_ptr<Camera> cam) {
        camera = cam;
    }

    // Texture management
    std::shared_ptr<Texture> createTexture(const char* imagePath);
    std::shared_ptr<Texture> getDefaultWhiteTexture() { return defaultWhiteTexture; }
    VkSampler getDefaultSampler() const { return defaultSampler; }
    
    // Texture array management
    size_t addTexture(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> getTexture(size_t index) const;
    size_t getTextureCount() const { return textures.size(); }
    
    // Descriptor management
    DescriptorAllocator* getDescriptorAllocator() const { return descriptorAllocator.get(); }


private:
    VkInstance instance{VK_NULL_HANDLE};
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    std::vector<const char*> instanceExtensions;
    VkPhysicalDevice physicalDevice{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VmaAllocator vmaAllocator{VK_NULL_HANDLE}; // Memory allocator
    PFN_vkCmdPipelineBarrier2KHR fp_vkCmdPipelineBarrier2{nullptr}; // Cached function pointer
    std::vector<const char*> validationLayers;
    VkDebugUtilsMessengerEXT debugMessenger{VK_NULL_HANDLE};
    bool enableValidationLayers{false};
    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    VkCommandPool commandPool{VK_NULL_HANDLE};
    VkCommandBuffer commandBuffer{VK_NULL_HANDLE};
    VkRenderPass renderPass{VK_NULL_HANDLE};
    VkExtent2D swapchainExtent{};
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    QueueFamilyIndices queueFamilyIndices;
    VkPipelineLayout opaquePipelineLayout{VK_NULL_HANDLE}; // Shared pipeline layout for opaque objects
    std::vector<UBO> globalUBO; // UBO for each swapchain image
    std::vector<VkDescriptorSet> globalDescriptorSets;
    
    // Swapchain framebuffers 
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkImageLayout> swapchainImageLayouts; // Track current layout of each swapchain image
    VkFormat swapchainImageFormat;
    std::vector<VkFramebuffer> framebuffers;
    uint32_t imageCount{MAX_FRAMES_IN_FLIGHT};

    // Synchronization objects
    VkSemaphore imageAvailableSemaphore{VK_NULL_HANDLE};
    VkSemaphore renderFinishedSemaphore{VK_NULL_HANDLE};
    VkFence inFlightFence{VK_NULL_HANDLE};
    uint32_t currentFrame = 0;


    VkRenderingAttachmentInfo colorAttachmentInfo{};
    
    std::unique_ptr<GraphicPipeline> meshPipeline;
    
    std::vector<Drawable> drawables;

    std::unique_ptr<DescriptorAllocator> descriptorAllocator;

    std::unique_ptr<DescriptorLayouts> descriptorLayouts;

    std::shared_ptr<Camera> camera;

    // Texture support
    VkSampler defaultSampler{VK_NULL_HANDLE};
    std::shared_ptr<Texture> defaultWhiteTexture;
    std::vector<std::shared_ptr<Texture>> textures; // Array of loaded textures


    void initLogicalDevice();
    bool createSwapchain();
    bool pickPhysicalDevice();
    bool recreateSwapchain();

    bool createCommandPool();
    bool createCommandBuffer();
    void recordCommandBuffer(uint32_t imageIndex);
    
    bool createSwapchainImageViews();
    bool createFramebuffers();

    bool createRenderPass();

    void drawMesh(VkCommandBuffer commandBuffer, Mesh* mesh);

    void createSemaphores();
    void createFences();


    void writeCameraUBO();
    

    void createUBOs();
    void cleanupUBOs();

    void createDescriptorSets();
    void cleanupDescriptorSets();

    // Texture management
    void createDefaultTextures();
    void cleanupTextures();


    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer cmd);

    // TODO: refactor to use this structure for frame data
    //     struct FrameData {
    //     VkCommandPool commandPool;
    //     VkCommandBuffer commandBuffer;

    //     VkSemaphore swapchainSemaphore; // "Image Available"
    //     VkSemaphore renderSemaphore;    // "Render Finished"

    //     VkFence renderFence;
    // };

    // std::array<FrameData, 2> _frames;
    // int _frameNumber{0}; 

    void transitionImage(VkCommandBuffer commandBuffer,
                         VkImage image,
                         VkImageLayout oldLayout,
                         VkImageLayout newLayout,
                         VkPipelineStageFlags2 srcStageMask,
                         VkPipelineStageFlags2 dstStageMask,
                         VkAccessFlags2 srcAccessMask,
                         VkAccessFlags2 dstAccessMask);

    VkPipelineLayout createPipelineLayout();
    std::shared_ptr<GraphicPipeline> createOpaquePipeline(VkPipelineLayout pipelineLayout, const char* vertexShaderPath, const char* fragmentShaderPath);
    std::shared_ptr<GraphicPipeline> createOpaquePipeline(const char* vertexShaderPath, const char* fragmentShaderPath); // Uses shared opaque pipeline layout
};