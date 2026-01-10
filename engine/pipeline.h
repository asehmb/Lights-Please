#pragma once

#include <vulkan/vulkan.h>
#include <vector>

// Forward declare GraphicPipeline for PipelineBuilder
class GraphicPipeline;

struct PipelineBuilder {
    // 1. Storage for the various stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    
    // 2. The "Fixed Function" state structs
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineLayout pipelineLayout;
    
    // 3. Render Metadata (What images are we drawing to?)
    VkPipelineRenderingCreateInfo renderInfo{};
    VkFormat colorAttachmentFormat;

    PipelineBuilder() { clear(); }

    void clear() {
        // Reset everything to "Safe Defaults"
        inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        colorBlendAttachment = {};
        multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        depthStencil = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        shaderStages.clear();
    }

    // --- FLAGS & SETTERS ---

    void set_shaders(VkShaderModule vert, VkShaderModule frag) {
        shaderStages.clear();
        shaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, 
                                 .stage = VK_SHADER_STAGE_VERTEX_BIT, .module = vert, .pName = "main" });
        shaderStages.push_back({ .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, 
                                 .stage = VK_SHADER_STAGE_FRAGMENT_BIT, .module = frag, .pName = "main" });
    }

    void set_input_topology(VkPrimitiveTopology topology) {
        inputAssembly.topology = topology;
    }

    void set_polygon_mode(VkPolygonMode mode) {
        rasterizer.polygonMode = mode;
        rasterizer.lineWidth = 1.0f;
    }

    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace) {
        rasterizer.cullMode = cullMode;
        rasterizer.frontFace = frontFace;
    }

    void disable_blending() {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
    }

    void enable_blending_additive() {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    }

    void enable_depthtest(bool depthWrite, VkCompareOp op) {
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = depthWrite ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = op;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
    }

    // New method to build a GraphicPipeline object
    GraphicPipeline build(VkDevice device, VkRenderPass pass, const char* vertexShaderPath, const char* fragmentShaderPath);
    
    // Keep the old method for backward compatibility, but rename it
    VkPipeline buildRawPipeline(VkDevice device, VkRenderPass pass);
};

class GraphicPipeline {
public:
    GraphicPipeline(VkDevice device,
                    VkRenderPass renderPass,
                    VkPipelineLayout pipelineLayout,
                    const char* vertexShaderPath,
                    const char* fragmentShaderPath);
    ~GraphicPipeline();
    void bind(VkCommandBuffer cmdBuffer);
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipelineLayout;

private:
    VkDevice m_device;
    VkShaderModule vertModule;
    VkShaderModule fragModule;
    
};