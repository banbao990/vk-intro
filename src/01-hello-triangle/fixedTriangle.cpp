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

void FixedTriangleApp::init_pipeline() {
    VkPipelineLayout tri_pipeline_layout = VK_NULL_HANDLE;
    add_pipeline_no_input("01-tri-fixed.vert.spv", "01-tri-fixed.frag.spv", nullptr, 0, false, _render_pass, 0, _tri_pipeline, tri_pipeline_layout);
}

void FixedTriangleApp::init_scenes() {}
