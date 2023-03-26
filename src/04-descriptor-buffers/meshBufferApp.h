#pragma once

#include "../common/rasterization/rasBufferApp.h"

class MeshBufferApp :public RasBufferApp {
public:
    MeshBufferApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer);
    virtual ~MeshBufferApp() override;
protected:
    // virtual void init_per_frame() override;
    // virtual void init() override;
    virtual bool deal_with_sdl_event(SDL_Event& e) override;
    // virtual void draw() override;
    // virtual void init_commands() override;
    // virtual void init_render_pass() override;
    // virtual void init_framebuffers() override;
    virtual void init_pipeline() override;
    // virtual void init_sync_structures() override;
    virtual void init_scenes() override;
    virtual void render() override;
    virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) override;
    virtual void init_descriptors() override;

private:
    static const int COLOR_TYPE = 3;
    bool _key_pressed = false;
    int _color_type = 0;

    Mesh _mesh{};
    VkPipeline _mesh_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _mesh_pipeline_layout = VK_NULL_HANDLE;
};

namespace MeshBufferAppTest {
    struct MeshPushConstant {
        int _type;
    };

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
