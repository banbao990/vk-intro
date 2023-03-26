#pragma once

#include "../rasTexApp.h"

// https://www.saschawillems.de/blog/2018/07/19/vulkan-input-attachments-and-sub-passes/
// 2 subpass
// subpass 0 (output: color + depth)
// subpass 1 (output: color)
// can be easily extended to multiple color attachments
class RasSubpassApp :public RasTexApp {
public:
    RasSubpassApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasTexApp(name, width, height, use_validation_layer) {}
    virtual ~RasSubpassApp() override;
protected:
    // virtual void init_per_frame() override;
    virtual void init() override;
    // virtual bool deal_with_sdl_event(SDL_Event& e) override;
    virtual void draw() override;
    // virtual void init_commands() override;
    virtual void init_render_pass() override;
    virtual void init_framebuffers() override;
    virtual void init_pipeline() override = 0;
    // virtual void init_sync_structures() override;
    virtual void init_scenes() override = 0;
    virtual void render() override = 0;
    // virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) override;
    // virtual void init_descriptors() override;

    const VkFormat _COLOR_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
    std::vector<FrameBufferAttachment> _color_attachment{};

    VkPipeline _subpass_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _subpass_pipeline_layout = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayout> _subpass_set_layout{};
private:
};