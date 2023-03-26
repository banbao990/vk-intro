#pragma once

#include "types.h"

namespace vkinit {
    /* Info */
    VkPhysicalDeviceShaderDrawParametersFeatures physical_device_shader_draw_parameters_features(VkBool32 shader_draw_parameters);

    VkCommandPoolCreateInfo command_pool_create_info(
        uint32_t queue_family_index,
        VkCommandPoolCreateFlags flags = 0
    );

    VkCommandBufferAllocateInfo command_buffer_allocate_info(
        VkCommandPool pool,
        uint32_t count = 1,
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    );

    VkCommandBufferBeginInfo command_buffer_begin_info(
        VkCommandBufferUsageFlags flags = 0
    );

    VkFramebufferCreateInfo framebuffer_create_info(
        VkRenderPass renderPass,
        VkExtent2D extent
    );

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

    VkSubmitInfo submit_info(VkCommandBuffer* cmd);

    VkPresentInfoKHR present_info();

    VkRenderPassBeginInfo renderpass_begin_info(
        VkRenderPass render_pass,
        VkExtent2D window_extent,
        VkFramebuffer framebuffer
    );

    VkShaderModuleCreateInfo shader_module_create_info();

    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
        VkShaderStageFlagBits stage,
        VkShaderModule shader_module
    );

    // VAO configuration in OpenGL
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info();

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_create_info(
        VkPrimitiveTopology topology
    );

    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
        VkPolygonMode polygonMode
    );

    // MSAA
    VkPipelineMultisampleStateCreateInfo pipeline_multisampling_state_create_info();

    VkPipelineViewportStateCreateInfo pipeline_viewport_create_info(VkViewport* viewport, VkRect2D* scissor);

    VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info();

    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info();

    // contains shader inputs of a given pipeline
    // it is where you would configure your push-constants and descriptor sets
    VkPipelineLayoutCreateInfo pipeline_layout_create_info();

    // data buffer
    VkBufferCreateInfo buffer_create_info(uint32_t size, VkBufferUsageFlags usage);

    // images
    // can not use VkImages directly, the VkImages have to go through a VkImageView
    VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent);
    VkImageViewCreateInfo image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags);

    // depth
    VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(
        bool b_depth_test, bool b_depth_write, VkCompareOp compare_op
    );

    // decriptor
    VkDescriptorPoolCreateInfo descriptor_pool_create_info();
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info();

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info();

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
        VkDescriptorType type,
        VkShaderStageFlags stage_flags,
        uint32_t binding
    );

    VkWriteDescriptorSet write_descriptor_buffer(
        VkDescriptorType type,
        VkDescriptorSet dst_set,
        VkDescriptorBufferInfo* buffer_info,
        uint32_t dst_binding
    );

    // texture
    VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode sampler_address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dst_set, VkDescriptorImageInfo* image_info, uint32_t binding);

    /* State */
    VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state();

    /* Barrier */
    VkImageMemoryBarrier image_memory_barrier(VkImage& image, VkImageSubresourceRange range,
        VkImageLayout old_layout, VkImageLayout new_layout,
        VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask);
}
