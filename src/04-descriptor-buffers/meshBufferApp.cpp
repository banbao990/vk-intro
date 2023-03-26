#include "meshBufferApp.h"
#include "../common/initializers.h"

#include <glm/gtx/transform.hpp>

MeshBufferApp::MeshBufferApp(const char* name, uint32_t width, uint32_t height, bool use_validation_layer) :RasBufferApp(name, width, height, use_validation_layer) {
    std::cout << "\tPress `Space` to change the color type!" << std::endl;
}

MeshBufferApp::~MeshBufferApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

bool MeshBufferApp::deal_with_sdl_event(SDL_Event& e) {
    // close the window when user alt-f4s or clicks the X button
    if (e.type == SDL_QUIT) {
        return true;
    }
    // space: change shader
    else if (e.type == SDL_KEYDOWN && !_key_pressed) {
        _key_pressed = true;
        if (e.key.keysym.sym == SDLK_SPACE) {
            _color_type = (_color_type + 1) % COLOR_TYPE;
        }
    } else if (e.type == SDL_KEYUP) {
        _key_pressed = false;
    }
    return false;
}

void MeshBufferApp::init_pipeline() {
    add_pipeline("04-mesh.vert.spv", "04-mesh.frag.spv", _set_layout.data(), _set_layout.size(), true, _render_pass, 0, _mesh_pipeline, _mesh_pipeline_layout);
}

void MeshBufferApp::init_scenes() {
    _mesh.load_from_obj("bunny.obj");
    upload_mesh(_mesh);
}

void MeshBufferApp::render() {
    FrameData& frame = get_current_frame();
    const int frame_idx = get_current_frame_idx();

    using namespace MeshBufferAppTest;
    const int padding_size = vkutils::padding(
        sizeof(GPUUniformData),
        _gpu_properties.limits.minUniformBufferOffsetAlignment
    );

    {
        // set uniform data
        glm::vec3 camera_pos = { -2.0f, -1.0f, -7.0f };
        glm::mat4 view = glm::translate(glm::mat4(1.0f), camera_pos);
        glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1700.0f / 900.0f, 0.1f, 200.0f);
        projection[1][1] *= -1;

        GPUCameraData camera_data = { view, projection, projection * view };
        GPUUniformData uniform_data = { camera_data };

        char* data = nullptr;
        vmaMapMemory(_allocator, _uniform_data_buffer._allocation, (void**)(&data));
        data += frame_idx * padding_size;
        memcpy(data, &uniform_data, sizeof(GPUUniformData));
        vmaUnmapMemory(_allocator, _uniform_data_buffer._allocation);
    }

    const int OBJ_NUM = 3;
    {
        // set storage data
        using namespace MeshBufferAppTest;
        void* data = nullptr;
        vmaMapMemory(_allocator, frame._object_buffer._allocation, &data);
        GPUObjectData* object_ssbo = (GPUObjectData*)data;
        glm::mat4 trans = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f));
        for (int i = 0; i < OBJ_NUM; ++i) {
            object_ssbo[i]._model_matrix = glm::scale(glm::translate(trans, glm::vec3(0.5f * (i * OBJ_NUM / 2), 0, 0)), glm::vec3(3.0f));
        }
        vmaUnmapMemory(_allocator, frame._object_buffer._allocation);
    }

    VkCommandBuffer& cmd = get_current_frame()._main_command_buffer;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline);

    // dynamic descriptors(uniform)
    uint32_t uniform_offset = padding_size * frame_idx;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline_layout, 0, 1, &frame._uniform_data_descriptor_set, 1, &uniform_offset);

    // (storage)
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _mesh_pipeline_layout, 1, 1, &frame._object_descriptor_set, 0, nullptr);

    // (push_constant)
    MeshPushConstant mesh_constants = {};
    mesh_constants._type = _color_type;
    vkCmdPushConstants(cmd, _mesh_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstant), &mesh_constants);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &_mesh._vertex_buffer._buffer, &offset);

    for (int i = 0; i < OBJ_NUM; ++i) {
        vkCmdDraw(cmd, _mesh._vertices.size(), 1, 0, i);
    }
}

void MeshBufferApp::set_shader_input(PipelineBuilder& builder, VkPipelineLayout& layout, VkDescriptorSetLayout* set_layout, uint32_t set_layout_count) {
    VkPipelineLayoutCreateInfo layout_create_info = vkinit::pipeline_layout_create_info();

    // push_constant
    VkPushConstantRange mesh_push_constant_range = {};
    // this push constant range starts at the beginning
    mesh_push_constant_range.offset = 0;
    mesh_push_constant_range.size = sizeof(MeshBufferAppTest::MeshPushConstant);
    mesh_push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layout_create_info.pPushConstantRanges = &mesh_push_constant_range;
    layout_create_info.pushConstantRangeCount = 1;

    // discriptor set
    layout_create_info.setLayoutCount = set_layout_count;
    layout_create_info.pSetLayouts = set_layout;

    // vertex buffer
    VK_CHECK(vkCreatePipelineLayout(_device, &layout_create_info, nullptr, &layout));
    builder._pipeline_layout = layout;
}

void MeshBufferApp::init_descriptors() {
    _descriptors.init_pool(_device);

    // [1] uniform data
    init_descriptors_for_uniform_data(sizeof(MeshBufferAppTest::GPUUniformData));

    // [2] object data
    init_descriptors_for_object_data(sizeof(MeshBufferAppTest::GPUObjectData));

    // [3] textures
    // init_descriptors_for_texture();

    _main_deletion_queue.push_function([&]() { _descriptors.destroy(); });
}
