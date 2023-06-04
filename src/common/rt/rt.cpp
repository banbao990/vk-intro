#include "rt.h"
#include "../common/initializers.h"
#include "../common/shader.h"
#include "shared_with_shaders.h" // TODO: merge to 1

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl2.h"
#include "VkBootstrap.h"
#include "tiny_obj_loader.h"
#include "rtHelper.h"

static const float sMoveSpeed = 2.0f;
static const float sAccelMult = 5.0f;
static const float sRotateSpeed = 0.25f;

static const vec3 sSunPos = vec3(0.4f, 0.45f, 0.55f);
static const float sAmbientLight = 0.1f;

void RTApp::init_imgui() {
    // 1: create descriptor pool for IMGUI
    // the size of the pool is very oversize, but it'mesh_idx copied from imgui demo itself.
    const int MAX_SIZE_FOR_IMGUI = 100;
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, MAX_SIZE_FOR_IMGUI },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_SIZE_FOR_IMGUI }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = MAX_SIZE_FOR_IMGUI * std::size(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    VkDescriptorPool imgui_pool;
    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imgui_pool));

    // 2: initialize imgui library
    // this initializes the core structures of imgui
    ImGui::CreateContext();
    // this initializes imgui for SDL
    ImGui_ImplSDL2_InitForVulkan(_window);

    // this initializes imgui for Vulkan
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = _instance;
    init_info.PhysicalDevice = _physical_device;
    init_info.Device = _device;
    init_info.Queue = _graphics_queue;
    init_info.DescriptorPool = imgui_pool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Subpass = 0;

    // BB Begin
    // create renderpass for IMGUI
    init_render_pass_for_imgui();
    init_framebuffers_for_imgui();

    // BB end

    ImGui_ImplVulkan_Init(&init_info, _render_pass_for_imgui);

    // execute a gpu command to upload imgui font textures
    immediate_submit(
        [&](VkCommandBuffer cmd) {
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
        }
    );

    // clear font textures from cpu data
    ImGui_ImplVulkan_DestroyFontUploadObjects();

    //add the destroy the imgui created structures
    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyDescriptorPool(_device, imgui_pool, nullptr);
            ImGui_ImplVulkan_Shutdown();
        }
    );
}


void RTApp::draw_imgui(VkCommandBuffer cmd) {
    VkRenderPassBeginInfo render_pass_info = vkinit::renderpass_begin_info(_render_pass_for_imgui, _window_extent, _framebuffers_for_imgui[_swapchain_image_index]);
    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // ui start
    ImGui::Text("FPS: %.2f", fps());
    ImGui::Text("SPP: %d", _spp);
    using Duration = std::chrono::duration<float>;
    auto delta = std::chrono::duration_cast<Duration>(_frame_time_samples.back() - _time_start);

    ImGui::Text("Time Elapsed: %.2f", delta.count());
    int id = 0;
    if (ImGui::CollapsingHeader("Camera")) {
        ++id;
        ImGui::PushID(id);
        ImGui::Checkbox("Enable Camera", &mCameraEnable);
        ImGui::PopID();
    }

    // ui end

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    vkCmdEndRenderPass(cmd);
}

void RTApp::init_render_pass_for_imgui() {
    // 1. Add color_attachment
    VkAttachmentDescription color_attachment = {};

    // the format needed by the swapchain
    color_attachment.format = _swapchain_image_format;
    // no MSAA
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    // clear when load when loading the attachment
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    // keep the attachment stored when the renderpass ends
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // do not care about stencil now
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // do not care
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    // after the renderpass ends, the image has to be on a layout ready for display
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // 2. Add subpass
    VkAttachmentReference color_attachment_ref = {};
    // attachment number will index into the pAttachments array in the parent renderpass itself
    color_attachment_ref.attachment = 0; // index = 0
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // 1 subpass
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    // 3. Add render pass
    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    // connect the color & depth attachment to the info
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;

    // connect the subpass to the info
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_render_pass_for_imgui));

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyRenderPass(_device, _render_pass_for_imgui, nullptr);
        }
    );

}

void RTApp::fill_rt_command_buffer(VkCommandBuffer cmd) {
    // update params
    UniformParams uniform_data = {};
    uniform_data.sunPosAndAmbient = vec4(sSunPos, sAmbientLight);
    vec2 moveDelta(0.0f, 0.0f);
    if (mWKeyDown) {
        moveDelta.y += 1.0f;
    }
    if (mSKeyDown) {
        moveDelta.y -= 1.0f;
    }
    if (mAKeyDown) {
        moveDelta.x -= 1.0f;
    }
    if (mDKeyDown) {
        moveDelta.x += 1.0f;
    }
    float dt = 0.0f;
    if (_frame_time_samples.size() >= 2) {
        auto delta = std::chrono::duration_cast<std::chrono::duration<float>>(_frame_time_samples.back() - mLastRec);
        dt = delta.count() / 2.0f;
    }
    moveDelta *= sMoveSpeed * dt * (mCtrlDown ? sAccelMult : 1.0f);
    mCamera.Move(moveDelta.x, moveDelta.y);

    if (mCamera.IsCameraChanged()) {
        _spp = 1;
        _time_start = _frame_time_samples.back();
    }

    uniform_data.camPos = vec4(mCamera.GetPosition(), 0.0f);
    uniform_data.camDir = vec4(mCamera.GetDirection(), 0.0f);
    uniform_data.camUp = vec4(mCamera.GetUp(), 0.0f);
    uniform_data.camSide = vec4(mCamera.GetSide(), 0.0f);
    uniform_data.camNearFarFov = vec4(mCamera.GetNearPlane(), mCamera.GetFarPlane(), Deg2Rad(mCamera.GetFovY()), 0.0f);
    uniform_data.accumulate_spp = _spp;
    uniform_data.random_seed = rand();
    mLastRec = _frame_time_samples.back();

    char* data = nullptr;
    vmaMapMemory(_allocator, _uniform_data_buffer._allocation, (void**)(&data));
    memcpy(data, &uniform_data, sizeof(UniformParams));
    vmaUnmapMemory(_allocator, _uniform_data_buffer._allocation);

    VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    // wait for offscreen image
    // TODO: need more specific cmd stage
    rt_utils::image_barrier(cmd,
        _offscreen_image[0]._image._image,
        subresource_range,
        0,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );

    rt_utils::image_barrier(cmd,
        _offscreen_image[1]._image._image,
        subresource_range,
        0,
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL
    );

    /// ray tracing
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rt_pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, _rt_pipeline_layout, 0, static_cast<uint32_t>(_rt_set.size()), _rt_set.data(), 0, 0);

    VkStridedDeviceAddressRegionKHR raygen_region = {
        _SBT.get_SBT_address(_device) + _SBT.get_raygen_offset(),
        _SBT.get_groups_stride(),
        _SBT.get_raygen_size()
    };

    VkStridedDeviceAddressRegionKHR missRegion = {
        _SBT.get_SBT_address(_device) + _SBT.get_miss_groups_offset(),
        _SBT.get_groups_stride(),
        _SBT.get_miss_groups_size()
    };

    VkStridedDeviceAddressRegionKHR hitRegion = {
       _SBT.get_SBT_address(_device) + _SBT.get_hit_groups_offset(),
        _SBT.get_groups_stride(),
        _SBT.get_hit_groups_size()
    };

    VkStridedDeviceAddressRegionKHR callable_region = {};

    _loader_manager->vkCmdTraceRaysKHR(cmd, &raygen_region, &missRegion, &hitRegion, &callable_region, _window_extent.width, _window_extent.height, 1u);

    // copy to swapchain
    // TODO: need more specific cmd stage
    rt_utils::image_barrier(cmd,
        _offscreen_image[0]._image._image,
        subresource_range,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    );

    VkImage swap = _swapchain_images[_swapchain_image_index];
    rt_utils::image_barrier(cmd,
        swap,
        subresource_range,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    VkImageCopy copy_region = {};
    copy_region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copy_region.srcOffset = { 0, 0, 0 };
    copy_region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
    copy_region.dstOffset = { 0, 0, 0 };
    copy_region.extent = { _window_extent.width, _window_extent.height, 1 };
    vkCmdCopyImage(cmd,
        _offscreen_image[0]._image._image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swap,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy_region
    );

    rt_utils::image_barrier(cmd,
        swap,
        subresource_range,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        0,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

    // draw imgui
    draw_imgui(cmd);
}

RTApp::RTApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :_frame_time_samples(30) {
    _window_extent.width = width;
    _window_extent.height = height;
    _name = name;
    _use_validation_layer = use_validation_layer;
    create_window(name, width, height);
}

RTApp::~RTApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RTApp::run() {
    init();

    // deal with SDL event
    SDL_Event e;
    bool b_quit = false;
    _spp = 0;
    _time_start = std::chrono::high_resolution_clock::now();

    // main loop
    while (!b_quit) {
        // FPS
        _frame_time_samples.push(std::chrono::high_resolution_clock::now());

        mCamera.SetCameraUnChanged();

        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            b_quit = deal_with_sdl_event(e);
        }
        init_per_frame();
        draw();
    }
}

void RTApp::init_per_frame() {
    ++_spp;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(_window);
    ImGui::NewFrame();
}

void RTApp::init() {
    init_vulkan();

    init_swapchain();

    init_offscreen_image();

    init_commands();

    init_sync_structures();

    init_scenes();

    init_descriptors();

    init_pipeline();

    init_imgui();

    update_descriptors();

    // everything went fine
    _is_initialized = true;
}

bool RTApp::deal_with_sdl_event(SDL_Event& e) {
    ImGui_ImplSDL2_ProcessEvent(&e);

    // close the window when user alt-f4s or clicks the X button
    if (e.type == SDL_QUIT) {
        return true;
    }

    if (!mCameraEnable) {
        return false;
    }

    // onkey
    if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_w: mWKeyDown = true; break;
        case SDLK_a: mAKeyDown = true; break;
        case SDLK_s: mSKeyDown = true; break;
        case SDLK_d: mDKeyDown = true; break;

        case SDLK_LCTRL:
        case SDLK_RCTRL:
            mCtrlDown = true;
            break;
        }
    }

    else if (e.type == SDL_KEYUP) {
        switch (e.key.keysym.sym) {
        case SDLK_w: mWKeyDown = false; break;
        case SDLK_a: mAKeyDown = false; break;
        case SDLK_s: mSKeyDown = false; break;
        case SDLK_d: mDKeyDown = false; break;

        case SDLK_LCTRL:
        case SDLK_RCTRL:
            mCtrlDown = false;
            break;
        }
    }

    // mouse
    else if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            mLMBDown = true;
        }
    }

    else if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_LEFT) {
            mLMBDown = false;
        }
    }

    // mouse move
    else if (e.type == SDL_MOUSEMOTION) {
        vec2 newPos(e.button.x, e.button.y);
        vec2 delta = mCursorPos - newPos;

        if (mLMBDown) {
            mCamera.Rotate(delta.x * sRotateSpeed, delta.y * sRotateSpeed);
        }

        mCursorPos = newPos;
    }

    return false;
}

void RTApp::draw() {
    // check if window is minimized and skip drawing
    if (SDL_GetWindowFlags(_window) & SDL_WINDOW_MINIMIZED) {
        return;
    }

    FrameData& frame = get_current_frame();

    // 1. check state
    // blocked or timeout
    // wait until the GPU has finished rendering the last frame.
    // timeout of 1 second
    VK_CHECK(vkWaitForFences(_device, 1, &frame._render_fence, true, 1'000'000'000));
    VK_CHECK(vkResetFences(_device, 1, &frame._render_fence)); // !!important!!

    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1'000'000'000, frame._present_semaphore, nullptr, &_swapchain_image_index));

    // 2. prepare command buffer

    // begin the command buffer recording.
    VkCommandBuffer cmd = frame._main_command_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // We will use this command buffer exactly once
    VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // fill ray-tracing cmd
    fill_rt_command_buffer(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    // 4. submit the cmd to GPU
    VkSubmitInfo submit_info = vkinit::submit_info(&cmd);

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.pWaitDstStageMask = &wait_stage;

    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &frame._present_semaphore;

    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &frame._render_semaphore;

    // submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit_info, frame._render_fence));

    // 5. display to the screen
    VkPresentInfoKHR present_info = vkinit::present_info();

    present_info.swapchainCount = 1;
    present_info.pSwapchains = &_swapchain;
    present_info.pImageIndices = &_swapchain_image_index;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &frame._render_semaphore;

    VK_CHECK(vkQueuePresentKHR(_graphics_queue, &present_info));

    ++_frame_number;
}

float RTApp::average_frame_time() {
    if (_frame_time_samples.size() <= 1) {
        return 0.0f;
    }
    using Duration = std::chrono::duration<float>;
    auto delta = std::chrono::duration_cast<Duration>(
        _frame_time_samples.back() - _frame_time_samples.front());
    return delta.count() / (float)(_frame_time_samples.size() - 1);
}

float RTApp::fps() {
    float frame_time = average_frame_time();
    if (frame_time == 0.0f) {
        return 0;
    } else {
        return 1.0f / frame_time;
    }
}

void RTApp::init_vulkan() {
    //VK_CHECK(volkInitialize());
    // 1. VkInstance
    // one process can only have one instance
    vkb::InstanceBuilder builder;
    builder.set_app_name(_name.c_str()).require_api_version(1, 3, 0); // Vulkan SDK is 1.3.236.0
    if (_use_validation_layer) {
        // for debug
        builder.request_validation_layers(true) // validation layer
            .use_default_debug_messenger();     // catches the log messages that the validation layers will output
    }
    // make the Vulkan instance
    vkb::Instance vkb_inst = builder.build().value();
    _instance = vkb_inst.instance;
    _debug_messager = vkb_inst.debug_messenger;

    //volkLoadInstance(_instance);
    // 2. VkSurface
    // get surface from the window
    SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

    // 3. VkPhysicalDevice
    vkb::PhysicalDeviceSelector selector(vkb_inst);

    std::vector<const char*> required_extensions({
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
            VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        }
    );

    vkb::PhysicalDevice physical_device = selector
        .add_required_extensions(required_extensions)
        .set_minimum_version(1, 3)
        .set_surface(_surface)
        .select()
        .value();

    _physical_device = physical_device.physical_device;
    _physical_device_properties = physical_device.properties;
    if (_physical_device_properties.limits.maxBoundDescriptorSets < SWS_NUM_SETS) {
        std::cerr
            << "Max Descriptor Sets should >= " << SWS_NUM_SETS
            << ", your limits is " << _physical_device_properties.limits.maxBoundDescriptorSets
            << std::endl;
    }

    // get ray-tracing properties
    _rt_properties = {};
    _rt_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    VkPhysicalDeviceProperties2 device_properties;
    device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    device_properties.pNext = &_rt_properties;
    device_properties.properties = {};
    vkGetPhysicalDeviceProperties2(_physical_device, &device_properties);

    // 4. VkDevice
    vkb::DeviceBuilder device_builder(physical_device);
    // ray tracing features
    // (1) to build acceleration structures
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    accel_feature.accelerationStructure = VK_TRUE;
    device_builder.add_pNext(&accel_feature);
    // (2) required by ray tracing pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt_pipeline_feature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    rt_pipeline_feature.rayTracingPipeline = VK_TRUE;
    device_builder.add_pNext(&rt_pipeline_feature);
    // TODO
    VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
    buffer_device_address.bufferDeviceAddress = VK_TRUE;
    device_builder.add_pNext(&buffer_device_address);
    // TODO
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
    //device_builder.add_pNext(&descriptor_indexing);

    VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features2.pNext = &descriptor_indexing;
    vkGetPhysicalDeviceFeatures2(_physical_device, &features2); // TODO: can enable all features(it and its pNext), now does not link them all
    device_builder.add_pNext(&features2);

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
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    VK_CHECK(vmaCreateAllocator(&allocator_info, &_allocator));
    _main_deletion_queue.push_function(
        [=]() {
            vmaDestroyAllocator(_allocator);
        }
    );

    // 7. loader
    _loader_manager = LoaderManager::get_instance();
    _loader_manager->init_extension_addr(_device);
}

void RTApp::init_swapchain() {
    vkb::SwapchainBuilder swapchain_builder(_physical_device, _device, _surface);

    VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    vkb::Swapchain swapchain = swapchain_builder
        .set_desired_format({ VK_FORMAT_R8G8B8A8_UNORM })
        .set_image_usage_flags(usage_flags)
        //.use_default_format_selection()
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
                vkDestroyImageView(_device, image_view, nullptr);
            }
            vkDestroySwapchainKHR(_device, _swapchain, nullptr);
        }
    );
}

void RTApp::flush_deletion_queue_and_vulkan_resources() {
    // keep the Surface, Device, Instance, and the SDL window out of the queue
    _main_deletion_queue.flush();

    vkDestroyDevice(_device, nullptr);

    // VkPhysicalDevice can't be destroyed, as it'mesh_idx not a Vulkan resource per-se,
    // it'mesh_idx more like just a handle to a GPU in the system.
    vkDestroySurfaceKHR(_instance, _surface, nullptr);

    vkb::destroy_debug_utils_messenger(_instance, _debug_messager);

    vkDestroyInstance(_instance, nullptr);

    SDL_DestroyWindow(_window);
}

void RTApp::init_sync_structures_for_graphics_pass() {
    // we want to create the fence with the Create Signaled flag,
    // so we can wait on it before using it on a GPU command (for the first frame)
    // initialized with signaled state
    VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& frame = _frames[i];
        VK_CHECK(vkCreateFence(_device, &fence_create_info, nullptr, &frame._render_fence));

        VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &frame._present_semaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphore_create_info, nullptr, &frame._render_semaphore));
    }

    _main_deletion_queue.push_function(
        [&]() {
            for (FrameData& frame : _frames) {
                vkDestroySemaphore(_device, frame._present_semaphore, nullptr);
                vkDestroySemaphore(_device, frame._render_semaphore, nullptr);
                vkDestroyFence(_device, frame._render_fence, nullptr);
            }
        }
    );
}

void RTApp::init_sync_structures() {
    init_sync_structures_for_graphics_pass();

    // immediately execute
    VkFenceCreateInfo upload_fence_create_info = vkinit::fence_create_info();
    VK_CHECK(vkCreateFence(_device, &upload_fence_create_info, nullptr, &_upload_context._upload_fence));
    _main_deletion_queue.push_function(
        [&]() {
            vkDestroyFence(_device, _upload_context._upload_fence, nullptr);
        }
    );
}

void RTApp::init_commands_for_graphics_pipeline() {
    // normal graphics pipeline
    // create cmd pool
    VkCommandPoolCreateInfo cmd_pool_info = vkinit::command_pool_create_info(
        _graphics_queue_family,
        // we also want the pool to allow for resetting of individual command buffers
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& frame = _frames[i];
        VK_CHECK(vkCreateCommandPool(_device, &cmd_pool_info, nullptr, &frame._command_pool));
        // Create Command Buffer
        VkCommandBufferAllocateInfo cmd_alloc_info = vkinit::command_buffer_allocate_info(frame._command_pool);
        VK_CHECK(vkAllocateCommandBuffers(_device, &cmd_alloc_info, &frame._main_command_buffer));
    }

    // resources
    _main_deletion_queue.push_function(
        [&]() {
            for (int i = 0; i < FRAME_OVERLAP; ++i) {
                FrameData& frame = _frames[i];
                // destroying their parent pool will destroy all of the command buffers allocated from it
                vkDestroyCommandPool(_device, frame._command_pool, nullptr);
            }
        }
    );
}

FrameData& RTApp::get_current_frame() {
    return _frames[get_current_frame_idx()];
}

uint32_t RTApp::get_current_frame_idx() const {
    return _frame_number % FRAME_OVERLAP;
}

void RTApp::basic_clean_up() {
    if (_is_initialized) {
        for (int i = 0; i < FRAME_OVERLAP; ++i) {
            FrameData& frame = _frames[i];
            VK_CHECK(vkWaitForFences(_device, 1, &frame._render_fence, true, 1'000'000'000));
        }
        VK_CHECK(vkDeviceWaitIdle(_device));
        flush_deletion_queue_and_vulkan_resources();
        _is_initialized = false;
    }
}

void RTApp::add_to_deletion_queue(std::function<void()>&& function) {
    _main_deletion_queue.push_function(std::move(function));
}

void RTApp::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VkCommandBuffer cmd = _upload_context._command_buffer;
    vkResetCommandPool(_device, _upload_context._command_pool, 0);

    // recording
    VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    // excecute
    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    // submit (run immediately)
    VkFence& fence = _upload_context._upload_fence;

    VkSubmitInfo submit_info = vkinit::submit_info(&cmd);
    VK_CHECK(vkQueueSubmit(_graphics_queue, 1, &submit_info, fence));

    vkWaitForFences(_device, 1, &fence, true, 1'000'000'000);
    vkResetFences(_device, 1, &fence);
}

void RTApp::create_window(const char* name, uint32_t width, uint32_t height) {
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

void RTApp::init_commands() {
    init_commands_for_graphics_pipeline();

    // immediately execute
    VkCommandPoolCreateInfo upload_cmd_pool_info = vkinit::command_pool_create_info(
        _graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
    );
    VK_CHECK(vkCreateCommandPool(_device, &upload_cmd_pool_info, nullptr, &_upload_context._command_pool));
    VkCommandBufferAllocateInfo upload_cmd_alloc_info = vkinit::command_buffer_allocate_info(_upload_context._command_pool);
    VK_CHECK(vkAllocateCommandBuffers(_device, &upload_cmd_alloc_info, &_upload_context._command_buffer));

    _main_deletion_queue.push_function(
        [&]() {
            vkDestroyCommandPool(_device, _upload_context._command_pool, nullptr);
        }
    );
}

void RTApp::init_offscreen_image() {
    VkExtent3D image_extent = {
        _window_extent.width,
        _window_extent.height,
        1
    };
    
    _offscreen_image.resize(2);
    std::vector<VkImageUsageFlags> usage_flags = {
        { VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT },
        { VK_IMAGE_USAGE_STORAGE_BIT }
    };
    
    for (int i = 0; i < _offscreen_image.size(); ++i) {
        FrameBufferAttachment& attach = _offscreen_image[i];
        AllocatedImage& image = attach._image;
        attach._format = _swapchain_image_format;

        VkImageCreateInfo image_info = vkinit::image_create_info(attach._format, usage_flags[i], image_extent);
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;

        VmaAllocationCreateInfo alloc_info = {};
        // only in GPU
        alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VK_CHECK(vmaCreateImage(_allocator, &image_info, &alloc_info, &image._image, &image._allocation, nullptr));

        VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageViewCreateInfo image_view_info = vkinit::image_view_create_info(attach._format, image._image, aspect_mask);
        VK_CHECK(vkCreateImageView(_device, &image_view_info, nullptr, &attach._image_view));

        _main_deletion_queue.push_function(
            [=]() {
                FrameBufferAttachment& attach = _offscreen_image[i];
                AllocatedImage& image = attach._image;

                vkDestroyImageView(_device, attach._image_view, nullptr);
                vmaDestroyImage(_allocator, image._image, image._allocation);
            }
        );
    }
}

void RTApp::init_framebuffers_for_imgui() {
    const uint32_t swapchain_image_count = (uint32_t)_swapchain_images.size();

    // create the framebuffers for the swapchain images.
    // This will connect the render-pass to the images for rendering
    VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_render_pass_for_imgui, _window_extent);

    // grab how many images we have in the swapchain
    _framebuffers_for_imgui = std::vector<VkFramebuffer>(swapchain_image_count);

    // create framebuffers for each of the swapchain image views
    for (uint32_t i = 0; i < swapchain_image_count; ++i) {
        VkImageView& image_view = _swapchain_image_views[i];

        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &image_view;

        VkFramebuffer& framebuffer = _framebuffers_for_imgui[i];
        VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &framebuffer));
    }

    _main_deletion_queue.push_function(
        [&]() {
            for (VkFramebuffer& framebuffer : _framebuffers_for_imgui) {
                vkDestroyFramebuffer(_device, framebuffer, nullptr);
            }
        }
    );
}

void RTApp::init_pipeline() {
    // 1. shaders and SBT
    VkShaderModule ray_gen_shader = Shader::load_shader_module(_device, "rt/ray_gen.rgen.bin");
    VkShaderModule ray_chit_shader = Shader::load_shader_module(_device, "rt/ray_chit.rchit.bin");
    VkShaderModule ray_miss_shader = Shader::load_shader_module(_device, "rt/ray_miss.rmiss.bin");
    VkShaderModule shadow_chit_shader = Shader::load_shader_module(_device, "rt/shadow_ray_chit.rchit.bin");
    VkShaderModule shadow_miss_shader = Shader::load_shader_module(_device, "rt/shadow_ray_miss.rmiss.bin");

    _SBT.initialize(2, 2, _rt_properties.shaderGroupHandleSize, _rt_properties.shaderGroupBaseAlignment);
    _SBT.set_raygen_stage(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_RAYGEN_BIT_KHR, ray_gen_shader));

    _SBT.add_stage_to_hit_groups(
        { vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, ray_chit_shader) },
        SWS_PRIMARY_HIT_SHADERS_IDX
    );
    _SBT.add_stage_to_hit_groups(
        { vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, shadow_chit_shader) },
        SWS_SHADOW_HIT_SHADERS_IDX
    );

    _SBT.add_stage_to_miss_groups(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_MISS_BIT_KHR, ray_miss_shader),
        SWS_PRIMARY_MISS_SHADERS_IDX
    );
    _SBT.add_stage_to_miss_groups(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_MISS_BIT_KHR, shadow_miss_shader),
        SWS_SHADOW_MISS_SHADERS_IDX
    );

    // 2. pipeline-layout
    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(_rt_set_layout.size());
    pipeline_layout_info.pSetLayouts = _rt_set_layout.data();
    VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_rt_pipeline_layout));

    // 3. pipeline
    VkRayTracingPipelineCreateInfoKHR rt_pipeline_info = { VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR, nullptr };
    rt_pipeline_info.stageCount = _SBT.get_num_stages();
    rt_pipeline_info.pStages = _SBT.get_stages();
    rt_pipeline_info.groupCount = _SBT.get_num_groups();
    rt_pipeline_info.pGroups = _SBT.get_groups();
    rt_pipeline_info.maxPipelineRayRecursionDepth = 1; // TODO
    rt_pipeline_info.layout = _rt_pipeline_layout;

    VK_CHECK(_loader_manager->vkCreateRayTracingPipelinesKHR(_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rt_pipeline_info, VK_NULL_HANDLE, &_rt_pipeline));

    // 4. shader binding table
    create_SBT();

    // shader module clean
    vkDestroyShaderModule(_device, ray_gen_shader, nullptr);
    vkDestroyShaderModule(_device, ray_chit_shader, nullptr);
    vkDestroyShaderModule(_device, ray_miss_shader, nullptr);
    vkDestroyShaderModule(_device, shadow_chit_shader, nullptr);
    vkDestroyShaderModule(_device, shadow_miss_shader, nullptr);

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyPipeline(_device, _rt_pipeline, nullptr);
            vkDestroyPipelineLayout(_device, _rt_pipeline_layout, nullptr);
        }
    );
}

void RTApp::create_SBT() {
    std::vector<unsigned char> group_handles(_SBT.get_num_groups() * _SBT.get_shader_handle_size());
    VK_CHECK(_loader_manager->vkGetRayTracingShaderGroupHandlesKHR(_device, _rt_pipeline, 0, _SBT.get_num_groups(), group_handles.size(), group_handles.data()));

    const uint32_t buffer_size = _SBT.get_SBT_size();

    // CPU Buffer ( for staging )
    AllocatedBuffer staging_buffer = rt_utils::create_buffer(_allocator, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // copy to CPU buffer
    {
        void* data;
        vmaMapMemory(_allocator, staging_buffer._allocation, &data);
        uint8_t* data_char = static_cast<uint8_t*>(data);
        const uint32_t group_num = _SBT.get_num_groups();
        const uint32_t shader_group_alignment = _SBT.get_shader_group_alignment();
        const uint32_t shader_handle_size = _SBT.get_shader_handle_size();
        for (size_t i = 0; i < group_num; ++i) {
            memcpy(data_char, group_handles.data() + i * shader_handle_size, shader_handle_size);
            data_char += shader_group_alignment;
        }
        vmaUnmapMemory(_allocator, staging_buffer._allocation);
    }
    // GPU buffer
    // creates a new VkBuffer, allocates and binds memory for it.
    AllocatedBuffer dst_buffer = rt_utils::create_buffer(_allocator,
        buffer_size,
        // TODO: why use this bit "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT"
        VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // do transfer immediately
    immediate_submit(
        [=](VkCommandBuffer cmd) {
            VkBufferCopy copy = {};
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = buffer_size;
            vkCmdCopyBuffer(cmd, staging_buffer._buffer, dst_buffer._buffer, 1, &copy);
        }
    );

    _SBT.set_SBT(dst_buffer);
    vmaDestroyBuffer(_allocator, staging_buffer._buffer, staging_buffer._allocation);

    _main_deletion_queue.push_function(
        [=]() {
            vmaDestroyBuffer(_allocator, dst_buffer._buffer, dst_buffer._allocation);
        }
    );
}

void RTApp::init_scenes() {
    mCamera.SetViewport({ 0, 0, static_cast<int>(_window_extent.width), static_cast<int>(_window_extent.height) });
    mCamera.SetViewPlanes(0.1f, 100.0f);
    mCamera.SetFovY(45.0f);
    mCamera.LookAt(vec3(0.25f, 3.20f, 6.15f), vec3(0.25f, 2.75f, 5.25f));

    // load scenes
    std::string path = std::string(ASSETS_DIRECTORY"/fake_whitted/fake_whitted.obj");

    // 1. tinyobj loading(upload the buffer to GPU when loading)
    tinyobj::attrib_t attrib;                       // vertex
    std::vector<tinyobj::shape_t> shapes;           // objects
    std::vector<tinyobj::material_t> materials;     // materials
    std::string warn, err;                          // loading info

    // assume the *.mtl file is in the same dir
    std::string base_dir = std::string(path);
    {
        size_t idx = base_dir.rfind('/');
        if (idx == std::string::npos) {
            base_dir = "";
        } else {
            base_dir = base_dir.substr(0, idx);
        }
    }

    // trianglulate = true by default
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), base_dir.c_str(), true);
    if (!warn.empty()) {
        std::cout << "[Obj Loading] " << path << ", Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cout << "[Obj Loading] " << path << ", Error: " << err << std::endl;
    }

    _rt_scene._meshes.resize(shapes.size());
    _rt_scene._materials.resize(materials.size());

    // upload
    for (size_t mesh_idx = 0; mesh_idx < shapes.size(); ++mesh_idx) {
        RTMesh& mesh = _rt_scene._meshes[mesh_idx];
        const tinyobj::shape_t& shape = shapes[mesh_idx];

        const size_t num_faces = shape.mesh.num_face_vertices.size();
        const size_t num_vertices = num_faces * 3;

        mesh._num_vertices = static_cast<uint32_t>(num_vertices);
        mesh._num_faces = static_cast<uint32_t>(num_faces);

        const size_t positions_buffer_size = num_vertices * sizeof(vec3);
        const size_t indices_buffer_size = num_vertices * sizeof(uint32_t);
        const size_t faces_buffer_size = num_faces * 4 * sizeof(uint32_t);
        const size_t attribs_buffer_size = num_vertices * sizeof(VertexAttribute);
        const size_t mat_IDs_buffer_size = num_faces * sizeof(uint32_t);

        // (1) create staging buffer
        const int BUFFER_KIND = 5;
        std::vector<AllocatedBuffer> staging_buffers(BUFFER_KIND);
        std::vector<void*> datas(BUFFER_KIND);
        std::vector<size_t> staging_buffer_size{ positions_buffer_size, indices_buffer_size, faces_buffer_size, attribs_buffer_size, mat_IDs_buffer_size };
        for (int buffer_idx = 0; buffer_idx < BUFFER_KIND; ++buffer_idx) {
            staging_buffers[buffer_idx] = rt_utils::create_buffer(_allocator, staging_buffer_size[buffer_idx], VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        }

        // (2) write to staging buffer
        // map
        for (int buffer_idx = 0; buffer_idx < BUFFER_KIND; ++buffer_idx) {
            vmaMapMemory(_allocator, staging_buffers[buffer_idx]._allocation, &datas[buffer_idx]);
        }
        vec3* data_positions = reinterpret_cast<vec3*>(datas[0]);
        uint32_t* data_indices = reinterpret_cast<uint32_t*>(datas[1]);
        uint32_t* data_faces = reinterpret_cast<uint32_t*>(datas[2]);
        VertexAttribute* data_attribs = reinterpret_cast<VertexAttribute*>(datas[3]);
        uint32_t* data_mat_IDs = reinterpret_cast<uint32_t*>(datas[4]);

        size_t vertex_idx = 0;
        for (size_t f = 0; f < num_faces; ++f) {
            assert(shape.mesh.num_face_vertices[f] == 3); // triangulate

            for (size_t j = 0; j < 3; ++j, ++vertex_idx) {
                const tinyobj::index_t& i = shape.mesh.indices[vertex_idx];

                vec3& pos = data_positions[vertex_idx];
                vec4& normal = data_attribs[vertex_idx].normal;
                vec4& uv = data_attribs[vertex_idx].uv;

                pos.x = attrib.vertices[3 * i.vertex_index + 0];
                pos.y = attrib.vertices[3 * i.vertex_index + 1];
                pos.z = attrib.vertices[3 * i.vertex_index + 2];
                normal.x = attrib.normals[3 * i.normal_index + 0];
                normal.y = attrib.normals[3 * i.normal_index + 1];
                normal.z = attrib.normals[3 * i.normal_index + 2];
                uv.x = attrib.texcoords[2 * i.texcoord_index + 0];
                uv.y = attrib.texcoords[2 * i.texcoord_index + 1];
            }

            const uint32_t a = static_cast<uint32_t>(3 * f + 0);
            const uint32_t b = static_cast<uint32_t>(3 * f + 1);
            const uint32_t c = static_cast<uint32_t>(3 * f + 2);
            data_indices[a] = a;
            data_indices[b] = b;
            data_indices[c] = c;
            data_faces[4 * f + 0] = a;
            data_faces[4 * f + 1] = b;
            data_faces[4 * f + 2] = c;

            data_mat_IDs[f] = static_cast<uint32_t>(shape.mesh.material_ids[f]);
        }

        // unmap
        for (int buffer_idx = 0; buffer_idx < BUFFER_KIND; ++buffer_idx) {
            vmaUnmapMemory(_allocator, staging_buffers[buffer_idx]._allocation);
        }

        // (3) create GPU buffer
        VkBufferUsageFlags basic_flags =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

        mesh._positions = rt_utils::create_buffer(_allocator, positions_buffer_size, basic_flags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
        mesh._indices = rt_utils::create_buffer(_allocator, indices_buffer_size, basic_flags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        basic_flags =
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        mesh._faces = rt_utils::create_buffer(_allocator, faces_buffer_size, basic_flags, VMA_MEMORY_USAGE_GPU_ONLY);
        mesh._attribs = rt_utils::create_buffer(_allocator, attribs_buffer_size, basic_flags, VMA_MEMORY_USAGE_GPU_ONLY);
        mesh._mat_IDs = rt_utils::create_buffer(_allocator, mat_IDs_buffer_size, basic_flags, VMA_MEMORY_USAGE_GPU_ONLY);

        const std::vector<AllocatedBuffer> target_buffer = { mesh._positions, mesh._indices, mesh._faces, mesh._attribs, mesh._mat_IDs };

        // (4) upload to GPU
        // TODO: multi mesh in one call
        immediate_submit(
            // TODO: single mesh in one call
            [=](VkCommandBuffer cmd) {
                VkBufferCopy copy = {};
                copy.srcOffset = 0;
                copy.dstOffset = 0;
                for (int buffer_idx = 0; buffer_idx < BUFFER_KIND; ++buffer_idx) {
                    copy.size = staging_buffer_size[buffer_idx];
                    vkCmdCopyBuffer(cmd, staging_buffers[buffer_idx]._buffer, target_buffer[buffer_idx]._buffer, 1, &copy);
                }
            }
        );

        // (5) clean staing buffer
        for (int buffer_idx = 0; buffer_idx < BUFFER_KIND; ++buffer_idx) {
            const AllocatedBuffer& buffer = staging_buffers[buffer_idx];
            vmaDestroyBuffer(_allocator, buffer._buffer, buffer._allocation);
        }

        _main_deletion_queue.push_function(
            [=]() {
                for (int buffer_idx = 0; buffer_idx < BUFFER_KIND; ++buffer_idx) {
                    const AllocatedBuffer& buffer = target_buffer[buffer_idx];
                    vmaDestroyBuffer(_allocator, buffer._buffer, buffer._allocation);
                }
            }
        );

        // (6) image data(the same process)
        // TODO: !!!IMPORTANT!!! should deal with the materials is lost situation
        std::vector<int> erase_materials{};
        for (size_t i = 0; i < materials.size(); ++i) {
            const tinyobj::material_t& src_mat = materials[i];
            RTMaterial& dst_mat = _rt_scene._materials[i];
            std::string full_texture_path = base_dir + "/" + src_mat.diffuse_texname;

            // load
            int width, height, channels;
            stbi_uc* pixels = stbi_load(full_texture_path.c_str(), &width, &height, &channels, STBI_rgb_alpha); // force RGBA
            if (!pixels) {
                std::cout << "[Image]: Failed to load " << full_texture_path << std::endl;
                erase_materials.push_back(i);
                continue;
            }
            int image_size = width * height * 4; // RGBA

            // upload to staging buffer
            const VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
            AllocatedBuffer staging_buffer = rt_utils::create_buffer(_allocator, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

            void* data;
            vmaMapMemory(_allocator, staging_buffer._allocation, &data);
            memcpy(data, pixels, image_size);
            vmaUnmapMemory(_allocator, staging_buffer._allocation);
            stbi_image_free(pixels);

            // create VkImage
            VkExtent3D image_extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
            VkImageCreateInfo image_create_info = vkinit::image_create_info(
                IMAGE_FORMAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_extent
            );

            VmaAllocationCreateInfo image_alloc_info = {};
            image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
            AllocatedImage& image = dst_mat._texture;
            VK_CHECK(vmaCreateImage(_allocator, &image_create_info, &image_alloc_info, &image._image, &image._allocation, nullptr));

            // layout transform
            immediate_submit(
                [&](VkCommandBuffer cmd) {
                    // set barrier
                    VkImageSubresourceRange range = {};
                    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    range.baseMipLevel = 0;
                    range.levelCount = 1;
                    range.baseArrayLayer = 0;
                    range.layerCount = 1;

                    VkImageMemoryBarrier image_barrier_to_transfer = vkinit::image_memory_barrier(
                        image._image, range,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // for transfer
                        0, VK_ACCESS_TRANSFER_WRITE_BIT
                    );
                    // https://gpuopen.com/learn/vulkan-barriers-explained/
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_transfer);

                    // copy
                    VkBufferImageCopy copy = {};
                    copy.bufferOffset = 0;
                    copy.bufferRowLength = 0;
                    copy.bufferImageHeight = 0;

                    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    copy.imageSubresource.baseArrayLayer = 0;
                    copy.imageSubresource.layerCount = 1;
                    copy.imageSubresource.mipLevel = 0;

                    copy.imageExtent = image_extent;

                    vkCmdCopyBufferToImage(cmd, staging_buffer._buffer, image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

                    // transfer the layout to be shader readable
                    VkImageMemoryBarrier image_barrier_to_readable = vkinit::image_memory_barrier(
                        image._image, range,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
                    );
                    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_readable);
                }
            );

            // image view
            VkImageView& image_view = dst_mat._image_view;
            VkImageViewCreateInfo image_view_info = vkinit::image_view_create_info(IMAGE_FORMAT, image._image, VK_IMAGE_ASPECT_COLOR_BIT);
            VK_CHECK(vkCreateImageView(_device, &image_view_info, nullptr, &image_view));

            VkSamplerCreateInfo sampler_create_info = vkinit::sampler_create_info(VK_FILTER_LINEAR);
            sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VK_CHECK(vkCreateSampler(_device, &sampler_create_info, nullptr, &dst_mat._sampler));

            _main_deletion_queue.push_function(
                [=]() {
                    vkDestroySampler(_device, dst_mat._sampler, nullptr);
                    vkDestroyImageView(_device, image_view, nullptr);
                    vmaDestroyImage(_allocator, image._image, image._allocation);
                }
            );

            // staging buffer
            vmaDestroyBuffer(_allocator, staging_buffer._buffer, staging_buffer._allocation);
        }
        for (int i = 0; i < erase_materials.size(); ++i) {
            _rt_scene._materials.erase(_rt_scene._materials.begin() + erase_materials[i] + i);
        }
    }

    std::cout << "[Obj Loading] successfully load \"" << path << "\"" << std::endl;

    // 2. shader info
    const size_t num_meshes = _rt_scene._meshes.size();
    const size_t num_materials = _rt_scene._materials.size();

    _rt_scene._mat_IDs_buffer_infos.resize(num_meshes);
    _rt_scene._attribs_buffer_infos.resize(num_meshes);
    _rt_scene._faces_buffer_infos.resize(num_meshes);
    for (size_t mesh_idx = 0; mesh_idx < num_meshes; ++mesh_idx) {
        const RTMesh& mesh = _rt_scene._meshes[mesh_idx];
        VkDescriptorBufferInfo& mat_IDs_info = _rt_scene._mat_IDs_buffer_infos[mesh_idx];
        VkDescriptorBufferInfo& attribs_info = _rt_scene._attribs_buffer_infos[mesh_idx];
        VkDescriptorBufferInfo& faces_info = _rt_scene._faces_buffer_infos[mesh_idx];

        mat_IDs_info.buffer = mesh._mat_IDs._buffer;
        mat_IDs_info.offset = 0;
        mat_IDs_info.range = mesh._mat_IDs._size;

        attribs_info.buffer = mesh._attribs._buffer;
        attribs_info.offset = 0;
        attribs_info.range = mesh._attribs._size;

        faces_info.buffer = mesh._faces._buffer;
        faces_info.offset = 0;
        faces_info.range = mesh._faces._size;
    }
    _rt_scene._textures_infos.resize(num_materials);
    for (size_t i = 0; i < num_materials; ++i) {
        const RTMaterial& mat = _rt_scene._materials[i];
        VkDescriptorImageInfo& textureInfo = _rt_scene._textures_infos[i];

        textureInfo.sampler = mat._sampler;
        textureInfo.imageView = mat._image_view;
        textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // 3. create scene
    // (3.1) scene
    _rt_scene.build_blas(_device, _allocator, this);
    _rt_scene.build_tlas(_device, _allocator, this);

    // (3.2) environment map 

    RTMaterial& dst_mat = _env_map;
    std::string full_texture_path = ASSETS_DIRECTORY"/envs/studio_garden_2k.jpg";

    // load
    int width, height, channels;
    stbi_uc* pixels = stbi_load(full_texture_path.c_str(), &width, &height, &channels, STBI_rgb_alpha); // force RGBA
    if (!pixels) {
        std::cout << "[Image]: Failed to load " << full_texture_path << std::endl;
    }

    assert(pixels != 0);

    int image_size = width * height * 4; // RGBA

    // upload to staging buffer
    const VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
    AllocatedBuffer staging_buffer = rt_utils::create_buffer(_allocator, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_allocator, staging_buffer._allocation, &data);
    memcpy(data, pixels, image_size);
    vmaUnmapMemory(_allocator, staging_buffer._allocation);
    stbi_image_free(pixels);

    // create VkImage
    VkExtent3D image_extent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    VkImageCreateInfo image_create_info = vkinit::image_create_info(
        IMAGE_FORMAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_extent
    );

    VmaAllocationCreateInfo image_alloc_info = {};
    image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    AllocatedImage& image = dst_mat._texture;
    VK_CHECK(vmaCreateImage(_allocator, &image_create_info, &image_alloc_info, &image._image, &image._allocation, nullptr));

    // layout transform
    immediate_submit(
        [&](VkCommandBuffer cmd) {
            // set barrier
            VkImageSubresourceRange range = {};
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = VK_REMAINING_MIP_LEVELS;
            range.baseArrayLayer = 0;
            range.layerCount = VK_REMAINING_ARRAY_LAYERS;

            VkImageMemoryBarrier image_barrier_to_transfer = vkinit::image_memory_barrier(
                image._image, range,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // for transfer
                0, VK_ACCESS_TRANSFER_WRITE_BIT
            );
            // https://gpuopen.com/learn/vulkan-barriers-explained/
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_transfer);

            // copy
            VkBufferImageCopy copy = {};
            copy.bufferOffset = 0;
            copy.bufferRowLength = 0;
            copy.bufferImageHeight = 0;

            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.layerCount = 1;
            copy.imageSubresource.mipLevel = 0;

            copy.imageExtent = image_extent;

            vkCmdCopyBufferToImage(cmd, staging_buffer._buffer, image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

            // transfer the layout to be shader readable
            VkImageMemoryBarrier image_barrier_to_readable = vkinit::image_memory_barrier(
                image._image, range,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT
            );
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier_to_readable);
        }
    );

    // image view & sampler
    VkImageViewCreateInfo image_view_info = vkinit::image_view_create_info(IMAGE_FORMAT, image._image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &image_view_info, nullptr, &dst_mat._image_view));

    VkSamplerCreateInfo sampler_create_info = vkinit::sampler_create_info(VK_FILTER_LINEAR);
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VK_CHECK(vkCreateSampler(_device, &sampler_create_info, nullptr, &dst_mat._sampler));

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroySampler(_device, dst_mat._sampler, nullptr);
            vkDestroyImageView(_device, dst_mat._image_view, nullptr);
            vmaDestroyImage(_allocator, image._image, image._allocation);
        }
    );

    // staging buffer
    vmaDestroyBuffer(_allocator, staging_buffer._buffer, staging_buffer._allocation);

    // shader info
    _env_map_info.sampler = dst_mat._sampler;
    _env_map_info.imageView = dst_mat._image_view;
    _env_map_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void RTApp::render() {
    std::cout << "render(): TODO" << std::endl;
}

void RTApp::init_descriptors() {
    const uint32_t num_meshes = static_cast<uint32_t>(_rt_scene._meshes.size());
    const uint32_t num_materials = static_cast<uint32_t>(_rt_scene._materials.size());

    // TODO: max = 4?
    // RTX3080: maxBoundDescriptorSets = 32
    _rt_set_layout.resize(SWS_NUM_SETS);
    _descriptors.init_pool(_device);

    // First set:
    //  binding 0  ->  AS
    //  binding 1  ->  Camera data
    //  binding 2  ->  output image
    //  binding 3  ->  accumulated image
    VkDescriptorType types0[] = {
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    };
    VkShaderStageFlags stages0[] = {
        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    };

    // binding number is ascending
    // SWS_SCENE_AS_BINDING     : 0
    // SWS_CAMDATA_BINDING      : 1
    // SWS_RESULT_IMAGE_BINDING : 2
    // SWS_ACCUMULATED_IMAGE_BINDING : 3
    _rt_set_layout[SWS_SCENE_AS_SET] = _descriptors.create_set_layout(types0, stages0, 4);

    // Second set:
    //  binding 0 (N)  ->  per-face material IDs for our meshes  (N = num meshes)
    const VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT; // binding.descriptorCount > 1, must set this flag
    VkDescriptorSetLayoutBindingFlagsCreateInfo flag1 = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
    flag1.pBindingFlags = &flag;
    flag1.bindingCount = 1;

    VkDescriptorSetLayoutBindingFlagsCreateInfo flags1[] = { flag1 };
    VkDescriptorType types1[] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
    VkShaderStageFlags stages1[] = { VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR };
    uint32_t descriptor_count1[] = { num_meshes };
    _rt_set_layout[SWS_MATIDS_SET] = _descriptors.create_set_layout(types1, stages1, 1, descriptor_count1, flags1);

    // Third set:
    //  binding 0 (N)  ->  vertex attributes for our meshes  (N = num meshes)
    //   (re-using second's set info)
    _rt_set_layout[SWS_ATTRIBS_SET] = _descriptors.create_set_layout(types1, stages1, 1, descriptor_count1, flags1);

    // Fourth set:
    //  binding 0 (N)  ->  faces info (indices) for our meshes  (N = num meshes)
    //   (re-using second's set info)
    _rt_set_layout[SWS_FACES_SET] = _descriptors.create_set_layout(types1, stages1, 1, descriptor_count1, flags1);

    // Fifth set:
    //  binding 0 (N)  ->  textures (N = num materials)
    VkDescriptorType types5[] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
    VkShaderStageFlags stages5[] = { VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR };
    uint32_t descriptor_count5[] = { num_materials };
    _rt_set_layout[SWS_TEXTURES_SET] = _descriptors.create_set_layout(types5, stages5, 1, descriptor_count5);


    // Sixth set:
    //  binding 0 ->  env texture
    VkDescriptorType types6[] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER };
    VkShaderStageFlags stages6[] = { VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR }; // TODO: 00000000 remove this stage
    _rt_set_layout[SWS_ENVS_SET] = _descriptors.create_set_layout(types6, stages6, 1);

    _main_deletion_queue.push_function(
        [&]() {
            _descriptors.destroy();
        }
    );
}

void RTApp::update_descriptors() {
    const uint32_t num_meshes = static_cast<uint32_t>(_rt_scene._meshes.size());
    const uint32_t num_materials = static_cast<uint32_t>(_rt_scene._materials.size());
    _rt_set.resize(_rt_set_layout.size());

    std::vector<uint32_t> desc_cnt = { 1, num_meshes, num_meshes, num_meshes, num_materials, 1 };
    assert(static_cast<uint32_t>(_rt_set_layout.size()) == static_cast<uint32_t>(desc_cnt.size()));
    _descriptors.create_set(_rt_set.data(), _rt_set_layout.data(), desc_cnt.data(), static_cast<uint32_t>(_rt_set_layout.size()));

    std::vector<VkWriteDescriptorSet> write_sets{};
    // First set:
    //  binding 0  ->  AS
    //  binding 1  ->  Camera data
    //  binding 2  ->  output image
    //  binding 3  ->  accumulated image
    VkWriteDescriptorSetAccelerationStructureKHR descriptor_as_info = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, nullptr };
    descriptor_as_info.accelerationStructureCount = 1;
    descriptor_as_info.pAccelerationStructures = &_rt_scene._tlas._acceleration_structure;

    VkWriteDescriptorSet ws = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    ws.pNext = &descriptor_as_info; // extension
    ws.dstSet = _rt_set[SWS_SCENE_AS_SET];
    ws.dstBinding = SWS_SCENE_AS_BINDING;
    ws.descriptorCount = 1;
    ws.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    write_sets.push_back(ws);
    // binding 0 end

    // TODO: only 1 buffer, do not need alignment
    const uint32_t padding_buffer_size = vkutils::padding(sizeof(UniformParams), _physical_device_properties.limits.minUniformBufferOffsetAlignment);
    const uint32_t total_buffer_size = padding_buffer_size;
    _uniform_data_buffer = rt_utils::create_buffer(_allocator, total_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkDescriptorBufferInfo uniform_data_info = {};
    uniform_data_info.buffer = _uniform_data_buffer._buffer;
    uniform_data_info.offset = 0;
    uniform_data_info.range = total_buffer_size;

    ws = vkinit::write_descriptor_buffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, _rt_set[SWS_SCENE_AS_SET], &uniform_data_info, SWS_CAMDATA_BINDING);
    write_sets.push_back(ws);
    // binding 1 end

    VkDescriptorImageInfo res_image_info = {};
    res_image_info.sampler = VK_NULL_HANDLE;
    res_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    res_image_info.imageView = _offscreen_image[0]._image_view;

    ws = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _rt_set[SWS_RESULT_IMAGE_SET], &res_image_info, SWS_RESULT_IMAGE_BINDING);
    write_sets.push_back(ws);
    // binding 2 end

    VkDescriptorImageInfo accu_image_info = {};
    accu_image_info.sampler = VK_NULL_HANDLE;
    accu_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    accu_image_info.imageView = _offscreen_image[1]._image_view;
    ws = vkinit::write_descriptor_image(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, _rt_set[SWS_ACCUMULATED_IMAGE_SET], &accu_image_info, SWS_ACCUMULATED_IMAGE_BINDING);
    write_sets.push_back(ws);
    // binding 3 end

    // Second set:
    //  binding 0 (N)  ->  per-face material IDs for our meshes  (N = num meshes)
    ws = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    ws.dstSet = _rt_set[SWS_MATIDS_SET];
    ws.dstBinding = 0;
    ws.descriptorCount = num_meshes;
    ws.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ws.pBufferInfo = _rt_scene._mat_IDs_buffer_infos.data();
    write_sets.push_back(ws);

    // Third set:
    //  binding 0 (N)  ->  vertex attributes for our meshes  (N = num meshes)
    //   (re-using second's set info)
    ws.dstSet = _rt_set[SWS_ATTRIBS_SET];
    ws.pBufferInfo = _rt_scene._attribs_buffer_infos.data();
    write_sets.push_back(ws);

    // Fourth set:
    //  binding 0 (N)  ->  faces info (indices) for our meshes  (N = num meshes)
    //   (re-using second's set info)
    ws.dstSet = _rt_set[SWS_FACES_SET];
    ws.pBufferInfo = _rt_scene._faces_buffer_infos.data();
    write_sets.push_back(ws);

    // Fifth set:
    //  binding 0 (N)  ->  textures (N = num materials)
    ws = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    ws.dstSet = _rt_set[SWS_TEXTURES_SET];
    ws.dstBinding = 0;
    ws.descriptorCount = num_materials;
    ws.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ws.pImageInfo = _rt_scene._textures_infos.data();
    write_sets.push_back(ws);

    // Sixth set:
    //  binding 0 ->  env texture
    ws = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    ws.dstSet = _rt_set[SWS_ENVS_SET];
    ws.dstBinding = 0;
    ws.descriptorCount = 1;
    ws.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ws.pImageInfo = &_env_map_info;
    write_sets.push_back(ws);

    _descriptors.bind(write_sets.data(), static_cast<uint32_t>(write_sets.size()));

    _main_deletion_queue.push_function(
        [&]() {
            vmaDestroyBuffer(_allocator, _uniform_data_buffer._buffer, _uniform_data_buffer._allocation);
        }
    );
}
