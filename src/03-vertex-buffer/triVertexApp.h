#pragma once

#include "../common/rasterization/rasVertexApp.h"

class TriVertexApp :public RasVertexApp {
public:
    TriVertexApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasVertexApp(name, width, height, use_validation_layer) {}
    virtual ~TriVertexApp() override;

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
    // virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) override;

private:
    Mesh _tri_mesh{};
    VkPipeline _tri_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _tri_pipeline_layout = VK_NULL_HANDLE;
};