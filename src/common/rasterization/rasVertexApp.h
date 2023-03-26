#pragma once

#include "rasDepthApp.h"
#include "../render.h"
#include "../pipeline.h"

class RasVertexApp : public RasDepthApp {
public:
    RasVertexApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasDepthApp(name, width, height, use_validation_layer) {}
    virtual ~RasVertexApp() override;

protected:
    // virtual void init_per_frame() override;
    // virtual void init() override;
    // virtual bool deal_with_sdl_event(SDL_Event& e) override;
    // virtual void draw() override;
    virtual void init_commands() override;
    // virtual void init_render_pass() override;
    // virtual void init_framebuffers() override;
    virtual void init_pipeline() = 0;
    virtual void init_sync_structures() override;
    virtual void init_scenes() override = 0;
    virtual void render() override = 0;
    // virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) override;

    void add_pipeline(
        const char* vertex_shader_relative_path,
        const char* fragment_shader_relative_path,
        VkDescriptorSetLayout* set_layout, uint32_t set_layout_count,
        bool use_z_buffer,
        VkRenderPass renderpass,
        uint32_t subpass,
        VkPipeline& pipeline, VkPipelineLayout& layout
    );
    AllocatedBuffer create_buffer(uint32_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);
    void upload_mesh(Mesh& mesh);
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

    // immediately execute
    ImmediateStructure _upload_context{};
private:
};