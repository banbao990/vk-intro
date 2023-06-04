#pragma once
//#define VK_NO_PROTOTYPES
//#include "volk/volk.h"

#include <vk_mem_alloc.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <deque>
#include <functional>
#include <string>
#include <chrono>
#include <iostream>

#include "../types.h"
#include "../utils.h"

#include "../descriptor.h"
#include "../pipeline.h"
#include "../data.h"
#include "../render.h"

#include "sbtHelper.h"
#include "rtHelper.h"
#include "camera.h"

#define NAME(X) #X
#define OUTPUT_KV(X) {                                                      \
    std::string xname = NAME(X);                                            \
    std::cout << xname.substr(2, xname.size() - 1) << ": " << X <<std::endl;\
}

class RTApp {
public:
    RTApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer);
    virtual ~RTApp();

    // run main loop
    void run();

protected:
    void init_per_frame();
    // initializes everything in the engine
    void init();
    // windows event, return true if want to quit
    bool deal_with_sdl_event(SDL_Event& e);
    // draw pre frame
    void draw();

    float average_frame_time();
    float fps();

    void init_vulkan();
    void init_swapchain();

    // must be called at the end of the cleanup (deconstructor)
    void flush_deletion_queue_and_vulkan_resources();

    std::string _name;
    uint32_t _frame_number = 0;
    bool _is_initialized{ false };

    // windows
    struct SDL_Window* _window = nullptr;
    VkExtent2D _window_extent{};

    // vulkan 
    VkPhysicalDevice _physical_device = VK_NULL_HANDLE;         // Vulkan physical device
    VkPhysicalDeviceProperties _physical_device_properties{};               // phisical device properties
    VkDevice _device = VK_NULL_HANDLE;                          // Vulkan device for commands
    VkInstance _instance = VK_NULL_HANDLE;                      // Vulkan library handle
    VkDebugUtilsMessengerEXT _debug_messager = VK_NULL_HANDLE;  // Vulkan debug output handle
    VkSurfaceKHR _surface = VK_NULL_HANDLE;                     // Vulkan window surface
    VmaAllocator _allocator = VK_NULL_HANDLE;                   // Memory allocator
    bool _use_validation_layer;

    // swapchain
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkFormat _swapchain_image_format = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> _swapchain_images{};
    std::vector<VkImageView> _swapchain_image_views{};
    uint32_t _swapchain_image_index = 0;

    // resource management
    DeletionQueue _main_deletion_queue{};

    // command queue
    VkQueue _graphics_queue = VK_NULL_HANDLE;              // queue we will submit to
    uint32_t _graphics_queue_family = 0;                   // family of that queue

    // FPS
    FixSizeQueue<std::chrono::high_resolution_clock::time_point> _frame_time_samples;

    void create_window(const char* name, uint32_t width, uint32_t height);
    // RasApp
    // command pool
    void init_commands();
    // set render target
    void init_render_pass();
    void init_framebuffers();
    void init_offscreen_image();
    virtual void init_pipeline();
    void init_sync_structures();
    virtual void init_scenes();
    virtual  void render();
    void set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count);

    void init_commands_for_graphics_pipeline();
    void add_pipeline_no_input(
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

    // RasDepthApp
    void add_attchment(FrameBufferAttachment* attachments, VkFormat format, VkImageUsageFlags usage_flag);
    const VkFormat _DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;
    std::vector<FrameBufferAttachment> _depth_attachment{};

    // RasVertexApp
    void add_pipeline(
        const char* vertex_shader_relative_path,
        const char* fragment_shader_relative_path,
        VkDescriptorSetLayout* set_layout, uint32_t set_layout_count,
        bool use_z_buffer,
        VkRenderPass renderpass,
        uint32_t subpass,
        VkPipeline& pipeline, VkPipelineLayout& layout
    );

    void upload_mesh(Mesh& mesh);

    // immediately execute
    ImmediateStructure _upload_context{};
    // RasBufferApp
    virtual void init_descriptors();
    virtual void update_descriptors();

    void init_descriptors_for_uniform_data(uint32_t buffer_size);
    void init_descriptors_for_object_data(uint32_t size_per_object);

    Descriptor _descriptors{};
    AllocatedBuffer _uniform_data_buffer{};

    // RasTexApp
    void init_imgui(uint32_t subpass, VkRenderPass render_pass);
    void upload_texture(Material& material);

    // Ray Tracing
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rt_properties{};
    std::vector<VkDescriptorSetLayout> _rt_set_layout{};
    std::vector<VkDescriptorSet> _rt_set{};

    FrameBufferAttachment _offscreen_image{};

    VkPipeline _rt_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _rt_pipeline_layout = VK_NULL_HANDLE;
    SBTHelper _SBT{};
    void fill_rt_command_buffer(VkCommandBuffer cmd);
    void create_SBT();

    RTScene _rt_scene{};
    RTMaterial _env_map{};
    VkDescriptorImageInfo _env_map_info{};

    LoaderManager* _loader_manager;

    // camera & user input
    Camera                          mCamera{};
    bool                            mWKeyDown{false};
    bool                            mAKeyDown{ false };
    bool                            mSKeyDown{ false };
    bool                            mDKeyDown{ false };
    bool                            mShiftDown{ false };
    bool                            mLMBDown{ false };
    vec2                            mCursorPos{};
    std::chrono::high_resolution_clock::time_point mLastRec{};
public:
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
    void add_to_deletion_queue(std::function<void()>&& function);
};