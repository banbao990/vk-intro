#include "pipeline.h"
#include "initializers.h"

#include <iostream>

VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass, bool use_z_buffer, uint32_t subpass) {
    // make viewport state from our stored viewport and scissor.
    // at the moment we won't support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewport_info =
        vkinit::pipeline_viewport_create_info(&_viewport, &_scissor);

    // related to the fragment shader output
    VkPipelineColorBlendStateCreateInfo color_blending =
        vkinit::pipeline_color_blend_state_create_info();
    // no blending (now no transparent objects)
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &_color_blend_attachment;

    VkGraphicsPipelineCreateInfo pipeline_info = vkinit::graphics_pipeline_create_info();

    pipeline_info.stageCount = (uint32_t)_shader_stages.size();
    pipeline_info.pStages = _shader_stages.data();
    pipeline_info.pVertexInputState = &_vertex_input_info;
    pipeline_info.pInputAssemblyState = &_input_assembly;

    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &_rasterizer;
    pipeline_info.pMultisampleState = &_multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.layout = _pipeline_layout;
    pipeline_info.renderPass = pass;
    pipeline_info.subpass = subpass;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (use_z_buffer) {
        pipeline_info.pDepthStencilState = &_depth_stencil;
    }

    VkPipeline new_pipeline;
    VkResult result = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline);
    if (result != VK_SUCCESS) {
        // failed to create graphics pipeline
        std::cout << "[ERROR] failed to create pipeline\n" << std::endl;
        return VK_NULL_HANDLE;
    } else {
        return new_pipeline;
    }
}

void PipelineBuilder::reset_shaders() {
    _shader_stages.clear();
}

void PipelineBuilder::add_shaders(VkShaderStageFlagBits stage, VkShaderModule shader_module) {
    _shader_stages.push_back(vkinit::pipeline_shader_stage_create_info(stage, shader_module));
}

