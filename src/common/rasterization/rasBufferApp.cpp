#include "rasBufferApp.h"
#include "../types.h"
#include "../initializers.h"

RasBufferApp::~RasBufferApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasBufferApp::init() {
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

    init_scenes();

    // everything went fine
    _is_initialized = true;
}

void RasBufferApp::set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) {
    VkPipelineLayoutCreateInfo layout_create_info = vkinit::pipeline_layout_create_info();

    // discriptor set
    layout_create_info.setLayoutCount = set_layout_count;
    layout_create_info.pSetLayouts = set_layout;

    // no push_constants
    VK_CHECK(vkCreatePipelineLayout(_device, &layout_create_info, nullptr, &layout));
    builder._pipeline_layout = layout;
}

void RasBufferApp::init_descriptors() {
    std::cerr << "You should implement the init_descriptor() funciton "
        "as this is just a sample code(size = 0 will error)"
        << std::endl;
    _descriptors.init_pool(_device);

    // [1] uniform data
    init_descriptors_for_uniform_data(0);

    // [2] object data
    init_descriptors_for_object_data(0);

    // [3] textures
    // init_descriptors_for_texture();

    _main_deletion_queue.push_function([&]() { _descriptors.destroy(); });
}

void RasBufferApp::init_descriptors_for_uniform_data(uint32_t buffer_size) {
    // allocate the descriotor-set-layout
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayout unifrom_data_set_layout = _descriptors.create_set_layout(&type, &stage);

    // allocate buffers
    const uint32_t padding_buffer_size = vkutils::padding(buffer_size, _physical_device_properties.limits.minUniformBufferOffsetAlignment);
    const uint32_t total_buffer_size = padding_buffer_size * FRAME_OVERLAP;
    _uniform_data_buffer = create_buffer(total_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& frame = _frames[i];

        // allcote the descriptor set (uniform data)
        frame._uniform_data_descriptor_set = _descriptors.create_set(unifrom_data_set_layout);

        // point to the buffers, allocate the buffers
        VkDescriptorBufferInfo uniform_buffer_info = {};
        uniform_buffer_info.buffer = _uniform_data_buffer._buffer;
        uniform_buffer_info.offset = 0; // dynamic
        uniform_buffer_info.range = buffer_size;

        _descriptors.bind(frame._uniform_data_descriptor_set, &uniform_buffer_info, type, 0);
    }

    _set_layout.push_back(unifrom_data_set_layout);

    _main_deletion_queue.push_function(
        [&]() {
            vmaDestroyBuffer(_allocator, _uniform_data_buffer._buffer, _uniform_data_buffer._allocation);
        }
    );
}

void RasBufferApp::init_descriptors_for_object_data(uint32_t size_per_object) {
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    VkShaderStageFlags stage = VK_SHADER_STAGE_VERTEX_BIT;
    VkDescriptorSetLayout object_set_layout = _descriptors.create_set_layout(&type, &stage);

    for (int i = 0; i < FRAME_OVERLAP; ++i) {
        FrameData& frame = _frames[i];

        // allocate buffers
        const int MAX_OBJECTS = 10'000;
        frame._object_buffer = create_buffer(
            size_per_object * MAX_OBJECTS,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        // allcote the descriptor set (object data)
        frame._object_descriptor_set = _descriptors.create_set(object_set_layout);

        VkDescriptorBufferInfo object_buffer_info = {};
        object_buffer_info.buffer = frame._object_buffer._buffer;
        object_buffer_info.offset = 0;
        object_buffer_info.range = size_per_object * MAX_OBJECTS;

        _descriptors.bind(frame._object_descriptor_set, &object_buffer_info, type, 0);
    }

    _set_layout.push_back(object_set_layout);

    _main_deletion_queue.push_function(
        [&]() {
            for (int i = 0; i < FRAME_OVERLAP; i++) {
                FrameData& frame = _frames[i];
                vmaDestroyBuffer(_allocator, frame._object_buffer._buffer, frame._object_buffer._allocation);
            }
        }
    );
}