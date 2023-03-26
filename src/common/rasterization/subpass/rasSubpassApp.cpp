#include "rasSubpassApp.h"
#include "initializers.h"
#include "../../shader.h"

#include <array>

RasSubpassApp::~RasSubpassApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasSubpassApp::init() {
    {
        VkPhysicalDeviceShaderDrawParametersFeatures features
            = vkinit::physical_device_shader_draw_parameters_features(VK_TRUE);
        init_vulkan(&features);
    }

    init_swapchain();

    init_render_pass();

    init_framebuffers();

    init_commands();

    init_descriptors();

    init_pipeline();

    init_sync_structures();

    init_imgui(1, _render_pass);

    init_scenes();

    // everything went fine
    _is_initialized = true;
}

void RasSubpassApp::draw() {
    // check if window is minimized and skip drawing
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) {
        return;
    }

    FrameData& frame = get_current_frame();

    // 1. check state
    // blocked or timeout
    // wait until the GPU has finished rendering the last frame.
    // timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &frame._render_fence, true, 1'000'000'000));
    VK_CHECK(vkResetFences(_device, 1, &frame._render_fence)); // !!important!!

    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1'000'000'000, frame._present_semaphore, nullptr, &_swapchain_image_index));

    // 2. prepare command buffer

    // begin the command buffer recording.
    VkCommandBuffer cmd = frame._main_command_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // We will use this command buffer exactly once
    VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // 3. add commands
    VkClearValue color_value{};
    color_value.color = { 0.0f, 0.0f, 0.0f, 0.0f };
    VkClearValue depth_value{};
    depth_value.depthStencil.depth = 1.0f; // max
    VkClearValue clear_values[3] = { color_value, depth_value, color_value };

    VkRenderPassBeginInfo rp_begin_info = vkinit::renderpass_begin_info(_render_pass, _window_extent, _framebuffers[_swapchain_image_index]);

    rp_begin_info.clearValueCount = 3;
    rp_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // rendering
    render();

    // finalize the render pass
    vkCmdEndRenderPass(cmd);
    // finalize the command buffer
    // (we can no longer add commands, but it can now be executed)
    VK_CHECK(vkEndCommandBuffer(cmd));

    // 4. submit the cmd to GPU
    VkSubmitInfo submit_info = vkinit::submit_info(&cmd);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask = &wait_stage; // TODO: complex concept

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame._present_semaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame._render_semaphore;

    // submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit_info, frame._render_fence));

    // 5. display to the screen
    VkPresentInfoKHR present_info = vkinit::present_info();

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;
    present_info.pImageIndices = &_swapchain_image_index;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame._render_semaphore;

    VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));

    ++_frame_number;
}

void RasSubpassApp::init_render_pass() {
    // [1] add attachment
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = _COLOR_FORMAT;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // subpass optimize, may not VK_ATTACHMENT_STORE_OP_STORE
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format = _DEPTH_FORMAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // subpass optimize, may not VK_ATTACHMENT_STORE_OP_STORE
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // do not use, VK_ATTACHMENT_STORE_OP_DONT_CARE is ok
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription swap_color_attachment = color_attachment;
    swap_color_attachment.format = _swapchain_image_format;
    swap_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swap_color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // [2] add subpass
    // subpass 0
    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass0 = {};
    subpass0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass0.colorAttachmentCount = 1;
    subpass0.pColorAttachments = &color_attachment_ref;
    subpass0.pDepthStencilAttachment = &depth_attachment_ref;

    // subpass 1
    VkAttachmentReference swap_color_attachment_ref = {};
    swap_color_attachment_ref.attachment = 2;
    swap_color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass1 = {};
    subpass1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass1.colorAttachmentCount = 1;
    subpass1.pColorAttachments = &swap_color_attachment_ref;

    // input_attachment (subpass0 -> subpass1)
    VkAttachmentReference input_attachment_color = {};
    input_attachment_color.attachment = 0;
    input_attachment_color.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkAttachmentReference input_attachment_depth = input_attachment_color;
    input_attachment_depth.attachment = 1;
    VkAttachmentReference input_attachments[2] = { input_attachment_color , input_attachment_depth };

    subpass1.inputAttachmentCount = 2;
    subpass1.pInputAttachments = input_attachments;

    // [3] add dependency (for layout transition)
    std::array<VkSubpassDependency, 3> dependencies = {};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT; // subpass input_attachment

    // This dependency transitions the input attachment from color attachment to shader read
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[2].srcSubpass = 1;
    dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // 3. Add render pass
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    render_pass_info.dependencyCount = (uint32_t)dependencies.size();
    render_pass_info.pDependencies = dependencies.data();

    // connect the color & depth attachment to the info
    // !!! the sequence are ordered !!!
    VkAttachmentDescription attachments[3] = { color_attachment, depth_attachment, swap_color_attachment };
    render_pass_info.attachmentCount = 3;
    render_pass_info.pAttachments = attachments;

    // connect the subpass to the info
    VkSubpassDescription subpasses[2] = { subpass0, subpass1 };
    render_pass_info.subpassCount = 2;
    render_pass_info.pSubpasses = subpasses;

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyRenderPass(_device, _render_pass, nullptr);
        }
    );
}

void RasSubpassApp::init_framebuffers() {
    const uint32_t swapchain_image_count = (uint32_t)_swapchain_images.size();
    _depth_attachment = std::vector<FrameBufferAttachment>(swapchain_image_count);
    // VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT flag is required for input attachments
    add_attchment(_depth_attachment.data(), _DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
    _color_attachment = std::vector<FrameBufferAttachment>(swapchain_image_count);
    add_attchment(_color_attachment.data(), _COLOR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);

    // create the framebuffers for the swapchain images.
    // This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_render_pass, _window_extent);

    // grab how many images we have in the swapchain
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    // create framebuffers for each of the swapchain image views
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkImageView& swap_chain_image_view = _swapchain_image_views[i];

        VkImageView image_views[3] = {
            _color_attachment[i]._image_view,
            _depth_attachment[i]._image_view,
            swap_chain_image_view
        };

        fb_info.attachmentCount = 3;
        fb_info.pAttachments = image_views;

        VkFramebuffer& framebuffer = _framebuffers[i];
        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &framebuffer));
    }

    _main_deletion_queue.push_function(
        [&]() {
            for (VkFramebuffer& framebuffer : _framebuffers) {
                vkDestroyFramebuffer(_device, framebuffer, nullptr);
            }
        }
    );
}