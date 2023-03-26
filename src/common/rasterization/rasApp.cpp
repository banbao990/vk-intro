#include "rasApp.h"
#include "../types.h"
#include "../initializers.h"
#include "../pipeline.h"
#include "../mesh.h"
#include "../shader.h"

RasApp::~RasApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasApp::init() {
    init_vulkan(nullptr);

    init_swapchain();

    init_render_pass();

    init_framebuffers();

    init_commands();

    init_pipeline();

    init_sync_structures();

    init_scenes();

    // everything went fine
    _is_initialized = true;
}

void RasApp::draw() {
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

    VkRenderPassBeginInfo rp_begin_info = vkinit::renderpass_begin_info(_render_pass, _window_extent, _framebuffers[_swapchain_image_index]);

    rp_begin_info.clearValueCount = 1;
    rp_begin_info.pClearValues = &color_value;

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

void RasApp::add_pipeline_no_input (
    const char* vertex_shader_relative_path,
    const char* fragment_shader_relative_path,
    VkDescriptorSetLayout* set_layout, uint32_t set_layout_count,
    bool use_z_buffer,
    VkRenderPass renderpass,
    uint32_t subpass,
    VkPipeline& pipeline, VkPipelineLayout& layout
) {
    // build the pipeline
    PipelineBuilder pipeline_builder;
    pipeline_builder.reset_shaders();

    // how to read vertex input (now no input)
    pipeline_builder._vertex_input_info = vkinit::pipeline_vertex_input_state_create_info();
    // drawing triangle lists, strips, or individual points
    pipeline_builder._input_assembly = vkinit::pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // viewport and scissor
    pipeline_builder._viewport.x = 0.0f;
    pipeline_builder._viewport.y = 0.0f;
    pipeline_builder._viewport.width = (float)_window_extent.width;
    pipeline_builder._viewport.height = (float)_window_extent.height;
    pipeline_builder._viewport.minDepth = 0.0f;
    pipeline_builder._viewport.maxDepth = 1.0f;

    pipeline_builder._scissor.offset = { 0,0 };
    pipeline_builder._scissor.extent = _window_extent;

    // wireframe or solid drawing
    pipeline_builder._rasterizer = vkinit::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    // no MSAA
    pipeline_builder._multisampling = vkinit::pipeline_multisampling_state_create_info();

    // no blending
    pipeline_builder._color_blend_attachment = vkinit::pipeline_color_blend_attachment_state();

    // z-buffer
    if (use_z_buffer) {
        pipeline_builder._depth_stencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    }

    Shader shader(_device, vertex_shader_relative_path, fragment_shader_relative_path);
    pipeline_builder.reset_shaders();
    pipeline_builder.add_shaders(VK_SHADER_STAGE_VERTEX_BIT, shader.vertex());
    pipeline_builder.add_shaders(VK_SHADER_STAGE_FRAGMENT_BIT, shader.fragment());

    // set layout
    set_shader_input(pipeline_builder, layout, set_layout, set_layout_count);
    pipeline_builder._pipeline_layout = layout;
    pipeline = pipeline_builder.build_pipeline(_device, renderpass, use_z_buffer, subpass);
    // cleanup
    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyPipeline(_device, pipeline, nullptr);
            vkDestroyPipelineLayout(_device, layout, nullptr);
        }
    );
}

void RasApp::set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) {
    VkPipelineLayoutCreateInfo layout_create_info = vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(_device, &layout_create_info, nullptr, &layout));
    builder._pipeline_layout = layout;
}

void RasApp::init_commands_for_graphics_pipeline() {
    // normal graphics pipeline
    // create cmd pool
    VkCommandPoolCreateInfo cmd_pool_info = vkinit::command_pool_create_info(
        _graphics_queue_family,
        // we also want the pool to allow for resetting of individual command buffers
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& frame = _frames[i];
        VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &frame._command_pool));
        // Create Command Buffer
        VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(frame._command_pool);
        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &frame._main_command_buffer));
    }

    // resources
    _main_deletion_queue.push_function(
        [&]() {
            for (int i = 0; i < FRAME_OVERLAP; ++i) {
                FrameData& frame = _frames[i];
                // destroying their parent pool will destroy all of the command buffers allocated from it
                vkDestroyCommandPool(_device, frame._command_pool, nullptr);
            }
        }
    );
}

void RasApp::init_sync_structures_for_graphics_pass() {
    // we want to create the fence with the Create Signaled flag,
    // so we can wait on it before using it on a GPU command (for the first frame)
    // initialized with signaled state
    VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& frame = _frames[i];
        VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &frame._render_fence));

        VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &frame._present_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &frame._render_semaphore));
    }

    _main_deletion_queue.push_function(
        [&]() {
            for (FrameData& frame : _frames) {
                vkDestroySemaphore(_device, frame._present_semaphore, nullptr);
                vkDestroySemaphore(_device, frame._render_semaphore, nullptr);
                vkDestroyFence(_device, frame._render_fence, nullptr);
            }
        }
    );
}

inline uint32_t RasApp::get_current_frame_idx() const {
    return _frame_number % FRAME_OVERLAP;
}

void RasApp::basic_clean_up() {
    if (_is_initialized) {
        for (int i = 0; i < FRAME_OVERLAP; ++i) {
            FrameData& frame = _frames[i];
            VK_CHECK(vkWaitForFences(_device, 1, &frame._render_fence, true, 1'000'000'000));
        }
        VK_CHECK(vkDeviceWaitIdle(_device));
        flushDeletionQueueAndVulkanResources();
        _is_initialized = false;
    }
}

FrameData& RasApp::get_current_frame() {
    return _frames[get_current_frame_idx()];
}

void RasApp::init_render_pass() {
    // 1. Add color_attachment
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

    // 2. Add subpass
    VkAttachmentReference color_attachment_ref = {};
    // attachment number will index into the pAttachments array in the parent renderpass itself
    color_attachment_ref.attachment = 0; // index = 0
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 1 subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    // 3. Add render pass
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    // connect the color & depth attachment to the info
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;

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
