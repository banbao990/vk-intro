#include "fixedTriangleDepth.h"
#include "../common/initializers.h"
#include "../common/pipeline.h"
#include "../common/shader.h"

void FixedTriangleDepthApp::render() {
    VkCommandBuffer& cmd = get_current_frame()._main_command_buffer;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _tri_pipeline);
    vkCmdDraw(cmd, (_tri_number + 1) * 3, 1, 0, 0);
}

FixedTriangleDepthApp::FixedTriangleDepthApp(
    const char* name, uint32_t width, uint32_t height, bool use_validation_layer
) :RasDepthApp(name, width, height, use_validation_layer) {
    std::cout << "\tPress `Space` to change the triangle numbers!" << std::endl;
}

FixedTriangleDepthApp::~FixedTriangleDepthApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

bool FixedTriangleDepthApp::deal_with_sdl_event(SDL_Event& e) {
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

void FixedTriangleDepthApp::init_commands() {
    init_commands_for_graphics_pipeline();
}

void FixedTriangleDepthApp::init_pipeline() {
    VkPipelineLayout tri_pipeline_layout = VK_NULL_HANDLE;
    add_pipeline_no_input("01-tri-fixed.vert.spv", "01-tri-fixed.frag.spv", nullptr, 0, true, _render_pass, 0, _tri_pipeline, tri_pipeline_layout);
}

void FixedTriangleDepthApp::init_sync_structures() {
    init_sync_structures_for_graphics_pass();
}

void FixedTriangleDepthApp::init_scenes() {}
