#pragma once

#include <vk_mem_alloc.h>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vector>
#include <deque>
#include <functional>
#include <string>
#include <chrono>
#include <iostream>

#include "types.h"
#include "utils.h"

class App {
public:
    App(const char* name, uint32_t width, uint32_t height, bool use_validation_layer = false);
    virtual ~App();

    // run main loop
    void run();

protected:
    virtual void init_per_frame() {};
    // initializes everything in the engine
    virtual void init();
    // windows event, return true if want to quit
    virtual bool deal_with_sdl_event(SDL_Event& e);
    // draw pre frame
    virtual void draw();

    float average_frame_time();
    float fps();

    void init_vulkan(VkPhysicalDeviceShaderDrawParametersFeatures* _shader_draw_parameters_feature = nullptr);
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
private:
};
