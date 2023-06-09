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
#include "ppg.h"

#define NAME(X) #X
#define OUTPUT_KV(X) {                                                      \
    std::string xname = NAME(X);                                            \
    std::cout << xname.substr(2, xname.size() - 1) << ": " << X <<std::endl;\
}

enum TEST_TYPE {
    NONE, EQUAL_SPP, EQUAL_TIME
};

class RTApp {
public:
    bool _ppg_on{ false };

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
    VkPhysicalDeviceProperties _physical_device_properties{};   // phisical device properties
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

    // command pool
    void init_commands();

    // imgui
    void init_imgui();
    void init_render_pass_for_imgui();
    void init_framebuffers_for_imgui();
    void draw_imgui(VkCommandBuffer cmd);

    VkRenderPass _render_pass_for_imgui = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> _framebuffers_for_imgui = {};

    void init_offscreen_image();
    void init_pipeline();
    void init_sync_structures();
    void init_scenes();

    void init_commands_for_graphics_pipeline();
    void init_sync_structures_for_graphics_pass();
    FrameData& get_current_frame();
    uint32_t get_current_frame_idx() const;
    void basic_clean_up();

    // frame Data
    static const uint32_t FRAME_OVERLAP = 2U; // TODO: !!!! error when set = 4
    FrameData _frames[FRAME_OVERLAP]{};

    // immediately execute
    ImmediateStructure _upload_context{};
    void init_descriptors();
    void update_descriptors();

    Descriptor _descriptors{};
    AllocatedBuffer _uniform_data_buffer{};

    uint32_t _radiance_cache_buffer_size{};
    AllocatedBuffer _radiance_cache_gpu{};
    AllocatedBuffer _radiance_cache_cpu{};

    // Ray Tracing
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR _rt_properties{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR _as_property{};
    std::vector<VkDescriptorSetLayout> _rt_set_layout{};
    std::vector<VkDescriptorSet> _rt_set{};

    // one for result, one for accumulate
    std::vector<FrameBufferAttachment> _offscreen_image{};

    VkPipeline _rt_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _rt_pipeline_layout = VK_NULL_HANDLE;
    SBTHelper _SBT{};
    void fill_rt_command_buffer(VkCommandBuffer cmd);
    void create_SBT();

    RTScene _rt_scene{};
    RTMaterial _env_map{};
    VkDescriptorImageInfo _env_map_info{};

    LoaderManager* _loader_manager;

    uint32_t _spp{ 0 };
    int _light_id{ 100 };
    int _glass_id{ 100 };
    int _mirror_id{ 100 };
    float _light_strength{ 1.0f };
    std::chrono::high_resolution_clock::time_point _time_start{};

    // camera & user input
    Camera                          mCamera{};
    bool                            mCameraEnable{ false };
    bool                            mWKeyDown{ false };
    bool                            mAKeyDown{ false };
    bool                            mSKeyDown{ false };
    bool                            mDKeyDown{ false };
    bool                            mCtrlDown{ false };
    bool                            mLMBDown{ false };
    vec2                            mCursorPos{};
    std::chrono::high_resolution_clock::time_point mLastRec{};

    // test
    TEST_TYPE _test_type{ NONE };
    bool _test_on{ false };
    bool _test_start{ false };
    int _test_spp{ 100 };
    std::chrono::high_resolution_clock::time_point _time_record_start{};
    float _test_time{ 5.0f };
    bool check_test_end();

    // ppg
    bool _ppg_train_on{ false };
    int _ppg_skip_first_data_obtain{ 2 }; // TODO: why skip 2
    bool _ppg_test_on{ false };
    bool _ppg_update_gpu_sdtree{ false };
    std::vector<STree> _stree{};
    std::vector<DTree> _dtree{};
    uint32_t _stree_buffer_size{};
    uint32_t _dtree_buffer_size{};
    AllocatedBuffer _stree_gpu{};
    AllocatedBuffer _stree_cpu{};
    AllocatedBuffer _dtree_gpu{};
    AllocatedBuffer _dtree_cpu{};

public:
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);
    void add_to_deletion_queue(std::function<void()>&& function);
    uint32_t get_min_acceleration_structure_scratch_offset_alignment();
};