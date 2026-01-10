
#include "pipeline.h"
#include "logger.h"
#include "geometry.h"
#include <vulkan/vulkan_core.h>
#include <fstream>
#include <stdexcept>

// Helper function to load shader code from file
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    return buffer;
}

// Helper function to create shader module
static VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    
    return shaderModule;
}

// GraphicPipeline implementation
GraphicPipeline::GraphicPipeline(VkDevice device,
                                VkRenderPass renderPass,
                                VkPipelineLayout pipelineLayout,
                                const char* vertexShaderPath,
                                const char* fragmentShaderPath)
    : m_device(device), m_pipelineLayout(pipelineLayout) {
    
    // Load and create shader modules
    auto vertShaderCode = readFile(vertexShaderPath);
    auto fragShaderCode = readFile(fragmentShaderPath);
    
    vertModule = createShaderModule(device, vertShaderCode);
    fragModule = createShaderModule(device, fragShaderCode);
    
    // Use PipelineBuilder to create the pipeline
    PipelineBuilder builder;
    builder.set_shaders(vertModule, fragModule);
    builder.set_input_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.set_polygon_mode(VK_POLYGON_MODE_FILL);
    builder.set_cull_mode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    builder.disable_blending();
    builder.enable_depthtest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.pipelineLayout = pipelineLayout;
    
    m_pipeline = builder.buildRawPipeline(device, renderPass);
}

GraphicPipeline::~GraphicPipeline() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
        }
        if (vertModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, vertModule, nullptr);
        }
        if (fragModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(m_device, fragModule, nullptr);
        }
    }
}

void GraphicPipeline::bind(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

GraphicPipeline PipelineBuilder::build(VkDevice device, VkRenderPass pass, const char* vertexShaderPath, const char* fragmentShaderPath) {
    return GraphicPipeline(device, pass, pipelineLayout, vertexShaderPath, fragmentShaderPath);
}

VkPipeline PipelineBuilder::buildRawPipeline(VkDevice device, VkRenderPass pass) {
    // 1. Setup Viewport/Scissor (Even if dynamic, we need the count)
    VkPipelineViewportStateCreateInfo viewportState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    // 2. Color Blending (Connects our attachment settings)
    VkPipelineColorBlendStateCreateInfo colorBlending{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    // 3. Dynamic State (So we can resize the window without crashing)
    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // 4. Vertex Input 
    static std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = Vertex::getAttributeDescriptions();
    VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();


    VkPipelineVertexInputStateCreateInfo vertexInputInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };

    // 5. THE MASTER STRUCT
    VkGraphicsPipelineCreateInfo pipelineInfo{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = pipelineLayout,
        .renderPass = pass,
        .subpass = 0
    };

    // 6. FINALLY: Create the GPU Object
    VkPipeline newPipeline;
    // TODO: change pipeline cache to use one later
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
        
        LOG_ERR("Failed to create graphics pipeline!");
        LOG_INFO("Pipeline layout: {}", static_cast<void*>(pipelineLayout));
        LOG_INFO("Render pass: {}", static_cast<void*>(pass));
        LOG_INFO("Subpass: {}", 0);
        throw std::runtime_error("Failed to create graphics pipeline!");
        return VK_NULL_HANDLE;
    }

    return newPipeline;
}