#pragma once

#include "../common/rasterization/rasTexApp.h"

class MeshTexApp :public RasTexApp {
public:
    MeshTexApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasTexApp(name, width, height, use_validation_layer) {}
    virtual ~MeshTexApp() override;
protected:
    // virtual void init_per_frame() override;
    // virtual void init() override;
    // virtual bool deal_with_sdl_event(SDL_Event& e) override;
    // virtual void draw() override;
    // virtual void init_commands() override;
    // virtual void init_render_pass() override;
    // virtual void init_framebuffers() override;
    virtual void init_pipeline() override;
    // virtual void init_sync_structures() override;
    virtual void init_scenes() override;
    virtual void render() override;
    // virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout set_layout, uint32_t set_layout_count) override;
    virtual void init_descriptors() override;

private:
    void draw_ui();
    void init_descriptors_for_texture();

    RenderObject _bunny{};
    ModelViewerCamera _camera{ 9.0f, 104.0f, 63.0f, -0.046f, 3.41f };
};

namespace MeshTexAppTest {
    struct GPUCameraData {
        glm::mat4 _view{};
        glm::mat4 _proj{};
        glm::mat4 _view_proj{}; // proj * view
    };

    struct GPUUniformData {
        GPUCameraData _camera{};
    };

    struct GPUObjectData {
        glm::mat4 _model_matrix{};
    };
}
