#include "rasDepthApp.h"
#include "../initializers.h"
#include "../pipeline.h"
#include "../shader.h"

RasDepthApp::~RasDepthApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasDepthApp::draw() {
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
    VkClearValue clear_values[2] = { color_value,depth_value };

    VkRenderPassBeginInfo rp_begin_info = vkinit::renderpass_begin_info(_render_pass, _window_extent, _framebuffers[_swapchain_image_index]);

    rp_begin_info.clearValueCount = 2;
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

void RasDepthApp::init_render_pass() {
    // 1. Add color_attachment & depth_attachment

    VkAttachmentDescription color_attachment = {};

    // the format needed by the swapchain
    color_attachment.format = _swapchain_image_format;
    // no MSAA
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // clear when load when loading the attachment
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // keep the attachment stored when the renderpass ends
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // do not care about stencil now
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // do not care
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // after the renderpass ends, the image has to be on a layout ready for display
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format = _DEPTH_FORMAT;
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 2. Add subpass

    VkAttachmentReference color_attachment_ref = {};
    // attachment number will index into the pAttachments array in the parent renderpass itself
    color_attachment_ref.attachment = 0; // index = 0
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref = {};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // 1 subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    // 3. Add render pass
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    // connect the color & depth attachment to the info
    VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };
    render_pass_info.attachmentCount = 2;
    render_pass_info.pAttachments = &attachments[0];

    // connect the subpass to the info
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass));

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyRenderPass(_device, _render_pass, nullptr);
        }
    );

}

void RasDepthApp::init_framebuffers() {
    const uint32_t swapchain_image_count = (uint32_t)_swapchain_images.size();
    _depth_attachment = std::vector<FrameBufferAttachment>(swapchain_image_count);;
    add_attchment(_depth_attachment.data(), VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // create the framebuffers for the swapchain images.
    // This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_render_pass, _window_extent);

    // grab how many images we have in the swapchain
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    // create framebuffers for each of the swapchain image views
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkImageView& image_view = _swapchain_image_views[i];

        VkImageView iv[2] = { image_view, _depth_attachment[i]._image_view};

        fb_info.attachmentCount = 2;
        fb_info.pAttachments = iv;

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

void RasDepthApp::add_attchment(FrameBufferAttachment* attachments, VkFormat format, VkImageUsageFlags usage_flag) {
    uint32_t swapchain_cnt = _swapchain_images.size();

    VkExtent3D image_extent = {
        _window_extent.width,
        _window_extent.height,
        1
    };

    for (int i = 0; i < swapchain_cnt; ++i) {
        FrameBufferAttachment& attach = attachments[i];
        AllocatedImage& image = attach._image;
        attach._format = format;

        VkImageAspectFlags aspect_mask = 0;

        if (usage_flag & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        if (usage_flag & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageCreateInfo image_info = vkinit::image_create_info(format, usage_flag, image_extent);

        VmaAllocationCreateInfo alloc_info = {};
        // only in GPU
        alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK(vmaCreateImage(_allocator, &image_info, &alloc_info, &image._image, &image._allocation, nullptr));

        VkImageViewCreateInfo image_view_info = vkinit::image_view_create_info(format, image._image, aspect_mask);
        VK_CHECK(vkCreateImageView(_device, &image_view_info, nullptr, &attach._image_view));
    }

    _main_deletion_queue.push_function(
        [=]() {
            for (int i = 0; i < swapchain_cnt; ++i) {
                FrameBufferAttachment& attach = attachments[i];
                AllocatedImage& image = attach._image;

                vkDestroyImageView(_device, attach._image_view, nullptr);
                vmaDestroyImage(_allocator, image._image, image._allocation);
            }
        }
    );
}