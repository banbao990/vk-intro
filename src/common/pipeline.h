#pragma once

#include <vector> 

#include "types.h"

class PipelineBuilder {
public:
    // basic set
    VkPipelineInputAssemblyStateCreateInfo _input_assembly = {};
    VkPipelineVertexInputStateCreateInfo _vertex_input_info = {};
    VkViewport _viewport = {};
    VkRect2D _scissor = {};
    VkPipelineRasterizationStateCreateInfo _rasterizer = {};
    VkPipelineMultisampleStateCreateInfo _multisampling = {};
    VkPipelineColorBlendAttachmentState _color_blend_attachment = {};
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;

    VkPipelineDepthStencilStateCreateInfo _depth_stencil = {};
    VkPipeline build_pipeline(VkDevice device, VkRenderPass pass, bool use_z_buffer, uint32_t subpass);

    // shaders
    void reset_shaders();
    void add_shaders(VkShaderStageFlagBits stage, VkShaderModule shader_module);
private:
    std::vector<VkPipelineShaderStageCreateInfo> _shader_stages{};
};