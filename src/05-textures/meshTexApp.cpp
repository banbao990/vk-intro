#include "meshTexApp.h"
#include "../common/initializers.h"

#include <glm/gtx/transform.hpp>

MeshTexApp::~MeshTexApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void MeshTexApp::init_pipeline() {
    add_pipeline("05-mesh.vert.spv", "05-mesh.frag.spv", _set_layout.data(), _set_layout.size(), true, _render_pass, 0, _bunny.pipeline(), _bunny.pipeline_layout());
}

void MeshTexApp::init_scenes() {
    _bunny._mesh.load_from_obj("bunny.obj");
    upload_mesh(_bunny._mesh);
    glm::mat4 trans = glm::mat4(1.0f);
    trans[1][1] = -1.0f; // flip
    _bunny._model_matrix = trans;
 
    _bunny._material.load_image_from_file("bunny.jpg");
    upload_texture(_bunny._material);

    // bind
    VkSampler tex_sampler = VK_NULL_HANDLE;
    VkSamplerCreateInfo sampler_info = vkinit::sampler_create_info(VK_FILTER_NEAREST);
    VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &tex_sampler));

    VkDescriptorImageInfo tex_info = {};
    tex_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex_info.imageView = _bunny.image_view();
    tex_info.sampler = tex_sampler;

    _descriptors.bind(_bunny.descriptor_set(), &tex_info, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0);

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroySampler(_device, tex_sampler, nullptr);
        }
    );
}

void MeshTexApp::render() {
    VkCommandBuffer& cmd = get_current_frame()._main_command_buffer;

    FrameData& frame = get_current_frame();
    const int frame_idx = get_current_frame_idx();

    using namespace MeshTexAppTest;
    const int padding_size = vkutils::padding(
        sizeof(GPUUniformData),
        _physical_device_properties.limits.minUniformBufferOffsetAlignment
    );

    {
        // set uniform data
        float aspect = (float)_window_extent.width / (float)_window_extent.height;
        glm::mat4 view = _camera.view();
        glm::mat4 projection = _camera.projection(aspect);

        GPUCameraData camera_data = { view, projection, projection * view };
        GPUUniformData uniform_data = { camera_data };

        char* data = nullptr;
        vmaMapMemory(_allocator, _uniform_data_buffer._allocation, (void**)(&data));
        data += frame_idx * padding_size;
        memcpy(data, &uniform_data, sizeof(GPUUniformData));
        vmaUnmapMemory(_allocator, _uniform_data_buffer._allocation);
    }
    {
        // set storage data
        using namespace MeshTexAppTest;
        void* data = nullptr;
        vmaMapMemory(_allocator, frame._object_buffer._allocation, &data);
        GPUObjectData* object_ssbo = (GPUObjectData*)data;
        object_ssbo[0]._model_matrix = _bunny._model_matrix;

        vmaUnmapMemory(_allocator, frame._object_buffer._allocation);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _bunny.pipeline());
    VkPipelineLayout& layout = _bunny.pipeline_layout();

    // uniform buffer (dynamic descriptors)
    uint32_t uniform_offset = padding_size * frame_idx;
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &frame._uniform_data_descriptor_set, 1, &uniform_offset);
    // storage buffer
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &frame._object_descriptor_set, 0, nullptr);
    // texture
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 2, 1, &_bunny.descriptor_set(), 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &_bunny._mesh._vertex_buffer._buffer, &offset);
    vkCmdDraw(cmd, _bunny._mesh._vertices.size(), 1, 0, 0);

    // draw ui
    draw_ui();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void MeshTexApp::init_descriptors() {
    _descriptors.init_pool(_device);

    using namespace MeshTexAppTest;

    // [1] uniform data
    init_descriptors_for_uniform_data(sizeof(GPUUniformData));

    // [2] object data
    init_descriptors_for_object_data(sizeof(GPUObjectData));

    // [3] textures
    init_descriptors_for_texture();

    _main_deletion_queue.push_function([&]() { _descriptors.destroy(); });
}

void MeshTexApp::init_descriptors_for_texture() {
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkShaderStageFlags stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayout tex_set_layout = _descriptors.create_set_layout(&type, &stage);

    VkDescriptorSet& desc_set = _bunny.descriptor_set();
    desc_set = _descriptors.create_set(tex_set_layout);

    // can not bind it, because the texture image is not loaded yet

    _set_layout.push_back(tex_set_layout);
}

void MeshTexApp::draw_ui() {
    ImGui::Text("FPS: %.2f", fps());
    ImGui::Text("Camera");
    _camera.draw_ui();
}
