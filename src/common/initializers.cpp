#include <initializers.h>

VkPhysicalDeviceShaderDrawParametersFeatures vkinit::physical_device_shader_draw_parameters_features(VkBool32 shader_draw_parameters) {
    VkPhysicalDeviceShaderDrawParametersFeatures features = {};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    features.pNext = nullptr;

    features.shaderDrawParameters = shader_draw_parameters;

    return features;
}

VkCommandPoolCreateInfo vkinit::command_pool_create_info(
    uint32_t queue_family_index,
    VkCommandPoolCreateFlags flags
) {
    VkCommandPoolCreateInfo info = {};

    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;

    // the command pool will be one that can submit graphics commands
    info.queueFamilyIndex = queue_family_index;
    info.flags = flags;

    return info;
}

VkCommandBufferAllocateInfo vkinit::command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count,
    VkCommandBufferLevel level
) {
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = pool;
    // we will allocate 1 command buffer
    info.commandBufferCount = count;
    // command level is primary
    info.level = level;

    return info;
}

VkCommandBufferBeginInfo vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext = nullptr;

    info.pInheritanceInfo = nullptr; // used for secondary cmd
    info.flags = flags;

    return info;
}

VkFramebufferCreateInfo vkinit::framebuffer_create_info(VkRenderPass render_pass, VkExtent2D extent) {
    VkFramebufferCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.pNext = nullptr;

    info.renderPass = render_pass;
    info.attachmentCount = 1;
    info.width = extent.width;
    info.height = extent.height;
    info.layers = 1;

    return info;
}

VkFenceCreateInfo vkinit::fence_create_info(VkFenceCreateFlags flags) {
    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags) {
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = nullptr;
    info.flags = flags;
    return info;
}

VkSubmitInfo vkinit::submit_info(VkCommandBuffer* cmd) {
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext = nullptr;

    info.commandBufferCount = 1;
    info.pCommandBuffers = cmd;

    info.pWaitDstStageMask = nullptr;

    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = nullptr;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores = nullptr;

    return info;
}

VkPresentInfoKHR vkinit::present_info() {
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.pNext = nullptr;

    info.swapchainCount = 0;
    info.pSwapchains = nullptr;

    info.pImageIndices = nullptr;

    info.pWaitSemaphores = nullptr;
    info.waitSemaphoreCount = 0;

    return info;
}

VkRenderPassBeginInfo vkinit::renderpass_begin_info(
    VkRenderPass render_pass,
    VkExtent2D window_extent,
    VkFramebuffer framebuffer
) {

    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.pNext = nullptr;

    info.renderPass = render_pass;

    info.renderArea.offset.x = 0;
    info.renderArea.offset.y = 0;
    info.renderArea.extent = window_extent;

    info.clearValueCount = 0;
    info.pClearValues = nullptr;

    info.framebuffer = framebuffer;

    return info;
}

VkShaderModuleCreateInfo vkinit::shader_module_create_info() {
    VkShaderModuleCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.pNext = nullptr;
    return info;
}

VkPipelineShaderStageCreateInfo vkinit::pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage,
    VkShaderModule shader_module
) {
    VkPipelineShaderStageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.stage = stage;
    // module containing the code for this shader stage
    info.module = shader_module;

    // !important
    // the entry point of the shader
    info.pName = "main";

    return info;
}

VkPipelineVertexInputStateCreateInfo vkinit::pipeline_vertex_input_state_create_info() {
    VkPipelineVertexInputStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.pNext = nullptr;

    // no vertex bindings or attributes
    info.vertexBindingDescriptionCount = 0;
    info.vertexAttributeDescriptionCount = 0;
    return info;
}

VkPipelineInputAssemblyStateCreateInfo vkinit::pipeline_input_assembly_create_info(
    VkPrimitiveTopology topology
) {
    VkPipelineInputAssemblyStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    info.pNext = nullptr;

    // what kind of topology will be drawn
    // normal triangle drawing, points, line-list, etc
    info.topology = topology;

    // we are not going to use primitive restart on the entire tutorial so leave it on false
    info.primitiveRestartEnable = VK_FALSE;

    return info;
}

VkPipelineRasterizationStateCreateInfo vkinit::pipeline_rasterization_state_create_info(
    VkPolygonMode polygonMode
) {
    VkPipelineRasterizationStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.depthClampEnable = VK_FALSE;
    // discards all primitives before the rasterization stage if enabled, which we don't want
    info.rasterizerDiscardEnable = VK_FALSE;

    // wireframe or solid drawing
    info.polygonMode = polygonMode;
    info.lineWidth = 1.0f;

    // no backface cull
    info.cullMode = VK_CULL_MODE_NONE;
    info.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // no depth bias
    info.depthBiasEnable = VK_FALSE;
    info.depthBiasConstantFactor = 0.0f;
    info.depthBiasClamp = 0.0f;
    info.depthBiasSlopeFactor = 0.0f;
    return info;
}

VkPipelineMultisampleStateCreateInfo vkinit::pipeline_multisampling_state_create_info() {
    VkPipelineMultisampleStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    info.pNext = nullptr;

    // default: no MSAA (1 spp)
    info.sampleShadingEnable = VK_FALSE;
    info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    info.minSampleShading = 1.0f;
    info.pSampleMask = nullptr;
    info.alphaToCoverageEnable = VK_FALSE;
    info.alphaToOneEnable = VK_FALSE;
    return info;
}

VkPipelineViewportStateCreateInfo vkinit::pipeline_viewport_create_info(
    VkViewport* viewport, VkRect2D* scissor
) {
    VkPipelineViewportStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    info.pNext = nullptr;

    info.viewportCount = 1;
    info.pViewports = viewport;
    info.scissorCount = 1;
    info.pScissors = scissor;

    return info;
}


VkPipelineColorBlendStateCreateInfo vkinit::pipeline_color_blend_state_create_info(){
    VkPipelineColorBlendStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    info.pNext = nullptr;

    return info;
}

VkGraphicsPipelineCreateInfo vkinit::graphics_pipeline_create_info() {
    VkGraphicsPipelineCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    info.pNext = nullptr;

    return info;
}


VkPipelineColorBlendAttachmentState vkinit::pipeline_color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    color_blend_attachment.blendEnable = VK_FALSE;
    return color_blend_attachment;
}

VkImageMemoryBarrier vkinit::image_memory_barrier(VkImage &image, VkImageSubresourceRange range,
    VkImageLayout old_layout, VkImageLayout new_layout,
    VkAccessFlags src_access_mask, VkAccessFlags dst_access_mask) {

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;

    barrier.image = image;
    barrier.subresourceRange = range;

    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    barrier.srcAccessMask = src_access_mask;
    barrier.dstAccessMask = dst_access_mask;

    return barrier;
}


VkPipelineLayoutCreateInfo vkinit::pipeline_layout_create_info() {
    VkPipelineLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // empty default
    info.flags = 0;
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    // no input
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;

    return info;
}

VkBufferCreateInfo vkinit::buffer_create_info(uint32_t size, VkBufferUsageFlags usage) {
    VkBufferCreateInfo info = {};

    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.pNext = nullptr;

    info.size = size;
    info.usage = usage;

    return info;
}

VkImageCreateInfo vkinit::image_create_info(VkFormat format, VkImageUsageFlags usage_flags, VkExtent3D extent) {
    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = extent;

    info.mipLevels = 1;                     // no mipmap
    info.arrayLayers = 1;                   // no layers (usage ex: cubemaps)
    info.samples = VK_SAMPLE_COUNT_1_BIT;   // no MSAA
    info.tiling = VK_IMAGE_TILING_OPTIMAL;  // how the data for the texture is arranged in the GPU
    info.usage = usage_flags;

    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 0

    return info;
}

VkImageViewCreateInfo vkinit::image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspect_flags) {
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.pNext = nullptr;

    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.image = image;
    info.format = format;

    // used for layers, point to which layer
    info.subresourceRange.baseMipLevel = 0;   // no mipmap
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0; // layer = 0
    info.subresourceRange.layerCount = 1;
    info.subresourceRange.aspectMask = aspect_flags;

    return info;
}

VkPipelineDepthStencilStateCreateInfo vkinit::depth_stencil_create_info(bool b_depth_test, bool b_depth_write, VkCompareOp compare_op) {
    VkPipelineDepthStencilStateCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    info.pNext = nullptr;

    // z-culling
    info.depthTestEnable = b_depth_test ? VK_TRUE : VK_FALSE;
    // write depth buffer
    info.depthWriteEnable = b_depth_write ? VK_TRUE : VK_FALSE;
    info.depthCompareOp = b_depth_test ? compare_op : VK_COMPARE_OP_ALWAYS;
    info.depthBoundsTestEnable = VK_FALSE;
    info.minDepthBounds = 0.0f; // Optional
    info.maxDepthBounds = 1.0f; // Optional
    info.stencilTestEnable = VK_FALSE;

    return info;
}

VkDescriptorPoolCreateInfo vkinit::descriptor_pool_create_info() {
    VkDescriptorPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    info.pNext = nullptr;

    info.flags = 0;

    return info;
}

VkDescriptorSetLayoutCreateInfo vkinit::descriptor_set_layout_create_info() {
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    return info;
}

VkDescriptorSetAllocateInfo vkinit::descriptor_set_allocate_info() {
    VkDescriptorSetAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    info.pNext = nullptr;

    return info;
}

VkDescriptorSetLayoutBinding vkinit::descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stage_flags, uint32_t binding) {
    VkDescriptorSetLayoutBinding set_bind = {};
    set_bind.binding = binding;
    set_bind.descriptorCount = 1;
    set_bind.descriptorType = type;
    set_bind.pImmutableSamplers = nullptr;
    set_bind.stageFlags = stage_flags;

    return set_bind;
}

VkWriteDescriptorSet vkinit::write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dst_set, VkDescriptorBufferInfo* buffer_info, uint32_t dst_binding) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = dst_binding;
    write.dstSet = dst_set;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = buffer_info;

    return write;
}

VkSamplerCreateInfo vkinit::sampler_create_info(VkFilter filters, VkSamplerAddressMode sampler_address_mode) {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = nullptr;

    info.magFilter = filters;
    info.minFilter = filters;
    info.addressModeU = sampler_address_mode;
    info.addressModeV = sampler_address_mode;
    info.addressModeW = sampler_address_mode;

    return info;
}

VkWriteDescriptorSet vkinit::write_descriptor_image(VkDescriptorType type, VkDescriptorSet dst_set, VkDescriptorImageInfo* image_info, uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;

    write.dstBinding = binding;
    write.dstSet = dst_set;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = image_info;

    return write;
}
