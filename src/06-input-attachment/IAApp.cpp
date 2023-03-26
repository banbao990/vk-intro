#include "IAApp.h"
#include "../common/initializers.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

IAApp::~IAApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void IAApp::init_pipeline() {
    add_pipeline("05-mesh.vert.spv", "05-mesh.frag.spv", _set_layout.data(), _set_layout.size(), true,_render_pass, 0, _bunny.pipeline(), _bunny.pipeline_layout());
    add_pipeline_no_input("06-quad.vert.spv", "06-input-attachment.frag.spv", _subpass_set_layout.data(), _subpass_set_layout.size(), false, _render_pass, 1, _subpass_pipeline, _subpass_pipeline_layout);
}

void IAApp::init_scenes() {
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

void IAApp::render() {
    VkCommandBuffer& cmd = get_current_frame()._main_command_buffer;

    FrameData& frame = get_current_frame();
    const int frame_idx = get_current_frame_idx();

    using namespace IAAppTest;
    const int padding_size = vkutils::padding(
        sizeof(GPUUniformData),
        _gpu_properties.limits.minUniformBufferOffsetAlignment
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
        using namespace IAAppTest;
        void* data = nullptr;
        vmaMapMemory(_allocator, frame._object_buffer._allocation, &data);
        GPUObjectData* object_ssbo = (GPUObjectData*)data;
        object_ssbo[0]._model_matrix = _bunny._model_matrix;

        vmaUnmapMemory(_allocator, frame._object_buffer._allocation);
    }

    // [1] subpass 0
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

    // [2] subpass 1
    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _subpass_pipeline);
    VkDescriptorSet& desc_set = _descriptor_set_subpass1[_swapchain_image_index];
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _subpass_pipeline_layout, 0, 1, &desc_set, 0, nullptr);
    vkCmdDraw(cmd, 3, 1, 0, 0);

    // draw ui
    draw_ui();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void IAApp::init_descriptors() {
    _descriptors.init_pool(_device);

    using namespace IAAppTest;

    // [1] uniform data
    init_descriptors_for_uniform_data(sizeof(GPUUniformData));

    // [2] object data
    init_descriptors_for_object_data(sizeof(GPUObjectData));

    // [3] textures + color attachment
    init_descriptors_for_texture();

    _main_deletion_queue.push_function([&]() { _descriptors.destroy(); });
}

void IAApp::init_descriptors_for_texture() {
    // subpass 0
    {
        const uint32_t SETS_NUM = 1;
        VkDescriptorType types[SETS_NUM] = {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        };
        VkShaderStageFlags stages[SETS_NUM] = {
            VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        VkDescriptorSetLayout tex_set_layout = _descriptors.create_set_layout(types, stages, SETS_NUM);
        VkDescriptorSet& desc_set = _bunny.descriptor_set();
        // [1] alloc
        desc_set = _descriptors.create_set(tex_set_layout);
        // [2] can not bind it, because the texture image is not loaded yet

        _set_layout.push_back(tex_set_layout);
    }
    // subpass 1
    {
        const uint32_t SETS_NUM = 2;
        VkDescriptorType types[SETS_NUM] = {
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        };
        VkShaderStageFlags stages[SETS_NUM] = {
            VK_SHADER_STAGE_FRAGMENT_BIT,
            VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        VkDescriptorSetLayout subpass_tex_set_layout = _descriptors.create_set_layout(types, stages, SETS_NUM);

        const uint32_t size = _swapchain_images.size();
        _descriptor_set_subpass1.resize(size);

        for (int i = 0; i < size; ++i) {
            // [1] alloc
            VkDescriptorSet desc_set = _descriptors.create_set(subpass_tex_set_layout);;
            _descriptor_set_subpass1[i] = desc_set;

            // [2] bind
            // color attachment
            VkDescriptorImageInfo attach_info = {};
            attach_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attach_info.imageView = _color_attachment[i]._image_view;
            attach_info.sampler = VK_NULL_HANDLE;
            _descriptors.bind(desc_set, &attach_info, types[0], 0);

            // depth attachment
            attach_info.imageView = _depth_attachment[i]._image_view;
            _descriptors.bind(desc_set, &attach_info, types[1], 1);
        }

        _subpass_set_layout.push_back(subpass_tex_set_layout);
    }
}

void IAApp::draw_ui() {
    ImGui::Text("FPS: %.2f", fps());
    int id = 0;
    if (ImGui::CollapsingHeader("Camera")) {
        ++id;
        ImGui::PushID(id);
        _camera.draw_ui();
        ImGui::PopID();
    }
}
