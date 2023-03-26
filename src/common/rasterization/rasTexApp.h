#pragma once

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include "rasBufferApp.h"
#include "../common/camera.h"

// 1. add camera
// 2. add imgui
// [1] must invoke `
//      ImGui::Render();
//      ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
// ` at the beginning of render()
// 
// [2] must invoke `
//      ImGui_ImplSDL2_ProcessEvent(&e);
// ` at the beginning of the deal_with_sdl_event()
// 
// [3] add you imgui function between `ImGui::NewFrame()`<draw()> and `ImGui::Render()`
class RasTexApp :public RasBufferApp {
public:
    RasTexApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasBufferApp(name, width, height, use_validation_layer) {}
    virtual ~RasTexApp() override;
protected:
    virtual void init_per_frame() override;
    virtual void init() override;
    virtual bool deal_with_sdl_event(SDL_Event& e) override;
    // virtual void draw() override;
    // virtual void init_commands() override;
    // virtual void init_render_pass() override;
    // virtual void init_framebuffers() override;
    virtual void init_pipeline() override = 0;
    // virtual void init_sync_structures() override;
    virtual void init_scenes() override = 0;
    virtual void render() override = 0;
    // virtual void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count);
    // virtual void init_descriptors();

    void init_imgui(uint32_t subpass, VkRenderPass render_pass);
    void upload_texture(Material &material);
private:
};