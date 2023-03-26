#include "fixedTriangle.h"
#include "../common/initializers.h"
#include "../common/pipeline.h"
#include "../common/shader.h"

void FixedTriangleApp::render() {
    VkCommandBuffer& cmd = get_current_frame()._main_command_buffer;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _tri_pipeline);
    vkCmdDraw(cmd, (_tri_number + 1) * 3, 1, 0, 0);
}

FixedTriangleApp::FixedTriangleApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasApp(name, width, height, use_validation_layer) {
    std::cout << "\tPress `Space` to change the triangle numbers!" << std::endl;
}

FixedTriangleApp::~FixedTriangleApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

bool FixedTriangleApp::deal_with_sdl_event(SDL_Event& e) {
    // close the window when user alt-f4s or clicks the X button
    if (e.type == SDL_QUIT) {
        return true;
    }
    // space: change shader
    else if (e.type == SDL_KEYDOWN && !_key_pressed) {
        _key_pressed = true;
        if (e.key.keysym.sym == SDLK_SPACE) {
            _tri_number = (_tri_number + 1) % MAX_TRI_NUM;
        }
    } else if (e.type == SDL_KEYUP) {
        _key_pressed = false;
    }
    return false;
}

void FixedTriangleApp::init_commands() {
    init_commands_for_graphics_pipeline();
}

void FixedTriangleApp::init_framebuffers() {
    // create the framebuffers for the swapchain images.
    // This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_render_pass, _window_extent);

    // grab how many images we have in the swapchain
    const uint32_t swapchain_image_count = (uint32_t)_swapchain_images.size();
    _framebuffers = std::vector<VkFramebuffer>(swapchain_image_count);

    // create framebuffers for each of the swapchain image views
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkImageView& image_view = _swapchain_image_views[i];
        fb_info.pAttachments = &image_view;
        fb_info.attachmentCount = 1;
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

void FixedTriangleApp::init_pipeline() {
    VkPipelineLayout tri_pipeline_layout = VK_NULL_HANDLE;
    add_pipeline_no_input("01-tri-fixed.vert.spv", "01-tri-fixed.frag.spv", nullptr, 0, false, _render_pass, 0, _tri_pipeline, tri_pipeline_layout);
}

void FixedTriangleApp::init_sync_structures() {
    init_sync_structures_for_graphics_pass();
}

void FixedTriangleApp::init_scenes() {}
