#pragma once

#include "rasVertexApp.h"
#include "../render.h"
#include "../descriptor.h"

class RasBufferApp :public RasVertexApp {
public:
    RasBufferApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasVertexApp(name, width, height, use_validation_layer) {}
    virtual ~RasBufferApp() override;
protected:
    // virtual void init_per_frame() override;
    virtual void init() override;
    // virtual bool deal_with_sdl_event(SDL_Event& e) override;
    // virtual void draw() override;
    // virtual void init_commands() override;
    // virtual void init_render_pass() override;
    // virtual void init_framebuffers() override;
    virtual void init_pipeline() override = 0;
    // virtual void init_sync_structures() override;
    virtual void init_scenes() override = 0;
    virtual void render() override = 0;
    virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) override;

    virtual void init_descriptors();

    void init_descriptors_for_uniform_data(uint32_t buffer_size);
    void init_descriptors_for_object_data(uint32_t size_per_object);

    // descriptors
    Descriptor _descriptors{};
    AllocatedBuffer _uniform_data_buffer{};
    std::vector<VkDescriptorSetLayout> _set_layout{};
private:
};