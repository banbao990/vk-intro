#pragma once
#include "rasApp.h"

class RasDepthApp :public RasApp {
public:
    RasDepthApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasApp(name, width, height, use_validation_layer) {}
    virtual ~RasDepthApp() override;
protected:
    // virtual void init_per_frame() override;
    // virtual void init() override;
    // virtual bool deal_with_sdl_event(SDL_Event& e)  override;
    virtual void draw() override;
    // virtual void init_commands() override;
    virtual void init_render_pass() override;
    virtual void init_framebuffers() override;
    virtual void init_pipeline() override = 0;
    // virtual void init_sync_structures() override;
    virtual void init_scenes() override = 0;
    virtual void render() override = 0;
    // virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) override;

    void add_attchment(FrameBufferAttachment* attachments, VkFormat format, VkImageUsageFlags usage_flag);

    // depth
    const VkFormat _DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    std::vector<FrameBufferAttachment> _depth_attachment{};
private:
};