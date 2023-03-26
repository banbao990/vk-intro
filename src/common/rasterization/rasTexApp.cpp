#include "rasTexApp.h"
#include "../common/initializers.h"

RasTexApp::~RasTexApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasTexApp::init_per_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame(_window);
    ImGui::NewFrame();
}

void RasTexApp::init() {
    {
        VkPhysicalDeviceShaderDrawParametersFeatures features
            = vkinit::physical_device_shader_draw_parameters_features(VK_TRUE);
        init_vulkan(&features);
    }

    init_swapchain();

    init_render_pass();

    init_framebuffers();

    init_commands();

    init_descriptors();

    init_pipeline();

    init_sync_structures();

    init_imgui(0, _render_pass);

    init_scenes();

    // everything went fine
    _is_initialized = true;
}

bool RasTexApp::deal_with_sdl_event(SDL_Event& e) {
    ImGui_ImplSDL2_ProcessEvent(&e);

    // close the window when user alt-f4s or clicks the X button
    if (e.type == SDL_QUIT) {
        return true;
    }
    return false;
}

void RasTexApp::init_imgui(uint32_t subpass, VkRenderPass render_pass) {
    // 1: create descriptor pool for IMGUI
   // the size of the pool is very oversize, but it's copied from imgui demo itself.
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
    init_info.Subpass = subpass;

    ImGui_ImplVulkan_Init(&init_info, render_pass);

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

void RasTexApp::upload_texture(Material& material) {
    // staging buffer
    int image_size = material._width * material._height * 4; // RGBA
    
    const VkFormat IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
    AllocatedBuffer staging_buffer = create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    void* data;
    vmaMapMemory(_allocator, staging_buffer._allocation, &data);
    memcpy(data, material._pixels, image_size);
    vmaUnmapMemory(_allocator, staging_buffer._allocation);
    material.free();

    // create VkImage
    VkExtent3D image_extent = {
        static_cast<uint32_t>(material._width),
        static_cast<uint32_t>(material._height),
        1
    };
    VkImageCreateInfo image_create_info = vkinit::image_create_info(
        IMAGE_FORMAT, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_extent
    );

    VmaAllocationCreateInfo image_alloc_info = {};
    image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    AllocatedImage& image = material._texture._image;
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
    VkImageView& image_view = material._texture._image_view;
    VkImageViewCreateInfo image_view_info = vkinit::image_view_create_info(IMAGE_FORMAT, image._image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(_device, &image_view_info, nullptr, &image_view));
    
    _main_deletion_queue.push_function(
        [&]() {
            vmaDestroyImage(_allocator, image._image, image._allocation);
            vkDestroyImageView(_device, image_view, nullptr);
        }
    );

    // no use
    vmaDestroyBuffer(_allocator, staging_buffer._buffer, staging_buffer._allocation);
}