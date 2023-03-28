#include "rasTwoPassApp.h"
#include "../../initializers.h"


RasTwoPassApp::~RasTwoPassApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasTwoPassApp::init() {
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

    init_imgui(0, _render_pass1);

    init_scenes();

    // everything went fine
    _is_initialized = true;
}

void RasTwoPassApp::draw() {
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

    render();

    VK_CHECK(vkEndCommandBuffer(cmd));

    // 4. submit the cmd to GPU
    VkSubmitInfo submit_info = vkinit::submit_info(&cmd);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask = &wait_stage;

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

void RasTwoPassApp::init_render_pass() {
    // pass 0
    {
        // [1] add attachment
        VkAttachmentDescription color_attachment = {};
        color_attachment.format = _COLOR_FORMAT;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // save it
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depth_attachment = {};
        depth_attachment.format = _DEPTH_FORMAT;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // save it
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // do not use, VK_ATTACHMENT_STORE_OP_DONT_CARE is ok
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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

        // 3. Add render pass
        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        // connect the color & depth attachment to the info
        // !!! the sequence are ordered !!!
        VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };
        render_pass_info.attachmentCount = 2;
        render_pass_info.pAttachments = attachments;

        // connect the subpass to the info
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass0;

        VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));

    }
    // pass 1
    {
        // [1] add attachment
        VkAttachmentDescription color_attachment = {};
        color_attachment.format = _swapchain_image_format;
        color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // save it
        color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // [2] add subpass
        // subpass 0
        VkAttachmentReference color_attachment_ref = {};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass0 = {};
        subpass0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass0.colorAttachmentCount = 1;
        subpass0.pColorAttachments = &color_attachment_ref;


        // 3. Add render pass
        VkRenderPassCreateInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

        // !!! the sequence are ordered !!!
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments = &color_attachment;

        // connect the subpass to the info
        render_pass_info.subpassCount = 1;
        render_pass_info.pSubpasses = &subpass0;

        VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass1));
    }

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyRenderPass(_device, _render_pass1, nullptr);
            vkDestroyRenderPass(_device, _render_pass, nullptr);
        }
    );
}

void RasTwoPassApp::init_framebuffers() {
    const uint32_t swapchain_image_count = (uint32_t)_swapchain_images.size();

    // add attachment
    _depth_attachment = std::vector<FrameBufferAttachment>(swapchain_image_count);
    add_attchment(_depth_attachment.data(), _DEPTH_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    // set sampled bit (pass 2 will sample it)
    _color_attachment = std::vector<FrameBufferAttachment>(swapchain_image_count);
    add_attchment(_color_attachment.data(), _COLOR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    // pass 0
    {
        VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_render_pass, _window_extent);
        _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);
        // create framebuffers for each of the swapchain image views
        for (uint32_t i = 0; i < swapchain_image_count; ++i) {
            VkImageView iv[2] = {
                _color_attachment[i]._image_view,
                _depth_attachment[i]._image_view
            };

            fb_info.attachmentCount = 2;
            fb_info.pAttachments = iv;

            VkFramebuffer& framebuffer = _framebuffers[i];
            VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &framebuffer));
        }
    }

    // pass 1
    {
        VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_render_pass1, _window_extent);
        _framebuffers_pass1 = std::vector<VkFramebuffer>(swapchain_image_count);
        // create framebuffers for each of the swapchain image views
        for (uint32_t i = 0; i < swapchain_image_count; ++i) {
            VkImageView& image_view = _swapchain_image_views[i];

            fb_info.attachmentCount = 1;
            fb_info.pAttachments = &image_view;

            VkFramebuffer& framebuffer = _framebuffers_pass1[i];
            VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &framebuffer));
        }
    }

    _main_deletion_queue.push_function(
        [&]() {
            for (VkFramebuffer& framebuffer : _framebuffers_pass1) {
                vkDestroyFramebuffer(_device, framebuffer, nullptr);
            }
            for (VkFramebuffer& framebuffer : _framebuffers) {
                vkDestroyFramebuffer(_device, framebuffer, nullptr);
            }
        }
    );
}

void RasTwoPassApp::render() {
    std::cout << "You may have to implement render(), you have to invoke vkCmdBeginRenderPass() & vkCmdEndRenderPass()" << std::endl;
}
