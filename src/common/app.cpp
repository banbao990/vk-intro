#include "app.h"
#include "config.h"
#include "initializers.h"

#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#endif
#include <vk_mem_alloc.h>
#include <glm/gtx/transform.hpp>

#include <VkBootstrap.h>
#include <string>

App::App(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :_frame_time_samples(30), _use_validation_layer(use_validation_layer) {
    _window_extent.width = width;
    _window_extent.height = height;
    _name = name;

    create_window(name, width, height);
}

void App::create_window(const char* name, uint32_t width, uint32_t height) {
    // We initialize SDL and create a window with it. 
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        name,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        _window_extent.width,
        _window_extent.height,
        window_flags
    );
}

void App::init() {
    // load the core Vulkan structure
    init_vulkan(nullptr);

    // create swapchain (for display)
    init_swapchain();

    // everything went fine
    _is_initialized = true;
}

bool App::deal_with_sdl_event(SDL_Event& e) {
    // close the window when user alt-f4s or clicks the X button
    if (e.type == SDL_QUIT) {
        return true;
    }
    return false;
}

void App::init_vulkan(VkPhysicalDeviceShaderDrawParametersFeatures* _shader_draw_parameters_feature
) {
    // 1. VkInstance
    // one process can only have one instance
    vkb::InstanceBuilder builder;
    builder.set_app_name(_name.c_str()).require_api_version(1, 1, 0); // Vulkan SDK is 1.3.236.0
    if (_use_validation_layer) {
        // for debug
        builder.request_validation_layers(true) // validation layer
            .use_default_debug_messenger();     // catches the log messages that the validation layers will output
    }

    // make the Vulkan instance
    vkb::Instance vkb_inst = builder.build().value();
    _instance = vkb_inst.instance;
    _debug_messager = vkb_inst.debug_messenger;

    // 2. VkSurface
    // get surface from the window
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // 3. VkPhysicalDevice
    vkb::PhysicalDeviceSelector selector(vkb_inst);
    vkb::PhysicalDevice physical_device = selector.set_minimum_version(1, 1)
        .set_surface(_surface)
        .select()
        .value();

    _physical_device = physical_device.physical_device;
    _gpu_properties = physical_device.properties;

    // 4. VkDevice
    vkb::DeviceBuilder device_builder(physical_device);
    if (_shader_draw_parameters_feature != nullptr) {
        device_builder.add_pNext(_shader_draw_parameters_feature);
    }

    vkb::Device vkb_device = device_builder.build().value();

    _device = vkb_device.device;

    // 5. Queue (for rendering)
    _graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    _graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    // 6. Memory Allocator
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.physicalDevice = _physical_device;
    allocator_info.device = _device;
    allocator_info.instance = _instance;
    VK_CHECK(vmaCreateAllocator(&allocator_info, &_allocator));
    _main_deletion_queue.push_function(
        [=]() {
            vmaDestroyAllocator(_allocator);
        }
    );
}

App::~App() {
    if (_is_initialized) {
        VK_CHECK(vkDeviceWaitIdle(_device));
        flushDeletionQueueAndVulkanResources();
    }
}

void App::flushDeletionQueueAndVulkanResources() {
    // keep the Surface, Device, Instance, and the SDL window out of the queue
    _main_deletion_queue.flush();

    vkDestroyDevice(_device, nullptr);

    // VkPhysicalDevice can't be destroyed, as it's not a Vulkan resource per-se,
    // it's more like just a handle to a GPU in the system.
    vkDestroySurfaceKHR(_instance, _surface, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debug_messager);

    vkDestroyInstance(_instance, nullptr);

    SDL_DestroyWindow(_window);
}

void App::draw() {
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) {
        return;
    }
    ++_frame_number;
}

void App::run() {
    init();

    // deal with SDL event
    SDL_Event e;
    bool b_quit = false;

    // main loop
    while (!b_quit) {
        // FPS
        _frame_time_samples.push(std::chrono::high_resolution_clock::now());

        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            b_quit = deal_with_sdl_event(e);
        }
        init_per_frame();
        draw();
    }
}

float App::average_frame_time() {
    if (_frame_time_samples.size() <= 1) {
        return 0.0f;
    }
    using Duration = std::chrono::duration<float>;
    auto delta = std::chrono::duration_cast<Duration>(
        _frame_time_samples.back() - _frame_time_samples.front());
    return delta.count() / (float)(_frame_time_samples.size() - 1);
}

float App::fps() {
    float frame_time = average_frame_time();
    if (frame_time == 0.0f) {
        return 0;
    } else {
        return 1.0f / frame_time;
    }
}

void App::init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder(_physical_device, _device, _surface);
    vkb::Swapchain swapchain = swapchain_builder
        .use_default_format_selection()
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)      // hard V-Sync
        //.set_desired_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR) // no V-Sync
        .set_desired_extent(_window_extent.width, _window_extent.height)
        .build()
        .value();

    // store swapchain and its related images
    _swapchain = swapchain.swapchain;
    _swapchain_images = swapchain.get_images().value();
    _swapchain_image_views = swapchain.get_image_views().value();
    _swapchain_image_format = swapchain.image_format;

    _main_deletion_queue.push_function(
        [&]() {
            for (VkImageView image_view : _swapchain_image_views) {
                // TODO: what about image
                vkDestroyImageView(_device, image_view, nullptr);
            }
            vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        }
    );
}