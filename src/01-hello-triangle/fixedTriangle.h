#pragma once

#include "../common/rasterization/rasApp.h"

class FixedTriangleApp : public RasApp {
public:
    FixedTriangleApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer);
    virtual ~FixedTriangleApp();
protected:
    // virtual void init_per_frame() override;
    // virtual void init() override;
    virtual bool deal_with_sdl_event(SDL_Event& e) override;
    // virtual void draw() override;
    virtual void init_commands() override;
    virtual void init_framebuffers() override;
    virtual void init_pipeline() override;
    virtual void init_sync_structures() override;
    virtual void init_scenes() override;
    virtual void render() override;

private:
    // pipeline
    VkPipeline _tri_pipeline = VK_NULL_HANDLE;

    bool _key_pressed = false;
    uint32_t _tri_number = 0;
    static const uint32_t MAX_TRI_NUM = 5;
};