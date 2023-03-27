#pragma once

#include "../app.h"
#include "../data.h"
#include "../pipeline.h"

class RasApp :public App {
public:
    RasApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :App(name, width, height, use_validation_layer) {}
    virtual ~RasApp() override;

protected:
    // virtual void init_per_frame() override;
    virtual void init() override;
    // virtual bool deal_with_sdl_event(SDL_Event& e)  override;
    virtual void draw() override;

    // command pool
    virtual void init_commands();
    // set render target
    virtual void init_render_pass();
    virtual void init_framebuffers();
    virtual void init_pipeline() = 0;
    virtual void init_sync_structures();
    virtual void init_scenes() = 0;
    virtual void render() = 0;
    virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count);

    void init_commands_for_graphics_pipeline();
    void add_pipeline_no_input (
        const char* vertex_shader_relative_path,
        const char* fragment_shader_relative_path,
        VkDescriptorSetLayout* set_layout, uint32_t set_layout_count,
        bool use_z_buffer,
        VkRenderPass renderpass,
        uint32_t subpass,
        VkPipeline& pipeline, VkPipelineLayout& layout
    );
    void init_sync_structures_for_graphics_pass();
    FrameData& get_current_frame();
    uint32_t get_current_frame_idx() const;
    void basic_clean_up();

    // frame Data
    static const uint32_t FRAME_OVERLAP = 2U;
    FrameData _frames[FRAME_OVERLAP]{};
    // render pass
    VkRenderPass _render_pass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> _framebuffers = {};
private:
};