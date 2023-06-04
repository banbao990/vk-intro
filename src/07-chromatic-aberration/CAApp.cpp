#include "CAApp.h"
#include "../common/initializers.h"

CAApp::~CAApp() {
    basic_clean_up();
}

void CAApp::init_pipeline() {
    add_pipeline("05-mesh.vert.spv", "05-mesh.frag.spv", _set_layout.data(), _set_layout.size(), true, _render_pass, 0, _bunny.pipeline(), _bunny.pipeline_layout());
    add_pipeline_no_input("06-quad.vert.spv", "07-chromatic-aberration.frag.spv", _set_layout_pass1.data(), _set_layout_pass1.size(), false, _render_pass1, 0, _pipeline_pass1, _pipeline_layout_pass1);
}

void CAApp::init_scenes() {
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

void CAApp::render() {
    FrameData& frame = get_current_frame();
    VkCommandBuffer& cmd = frame._main_command_buffer;
    const int frame_idx = get_current_frame_idx();

    using namespace CAAppTest;
    const int padding_size = vkutils::padding(
        sizeof(GPUUniformData),
        _physical_device_properties.limits.minUniformBufferOffsetAlignment
    );

    // set buffer
    {
        {
            // set uniform data
            float aspect = (float)_window_extent.width / (float)_window_extent.height;
            glm::mat4 view = _camera.view();
            glm::mat4 projection = _camera.projection(aspect);

            GPUCameraData camera_data = { view, projection, projection * view };
            GPUUniformData uniform_data = { camera_data, _CA_strength };

            char* data = nullptr;
            vmaMapMemory(_allocator, _uniform_data_buffer._allocation, (void**)(&data));
            data += frame_idx * padding_size;
            memcpy(data, &uniform_data, sizeof(GPUUniformData));
            vmaUnmapMemory(_allocator, _uniform_data_buffer._allocation);
        }
        {
            // set storage data
            using namespace CAAppTest;
            void* data = nullptr;
            vmaMapMemory(_allocator, frame._object_buffer._allocation, &data);
            GPUObjectData* object_ssbo = (GPUObjectData*)data;
            object_ssbo[0]._model_matrix = _bunny._model_matrix;
            vmaUnmapMemory(_allocator, frame._object_buffer._allocation);
        }
    }

    // pass 0
    {
        VkClearValue color_value{};
        color_value.color = { 0.0f, 0.0f, 0.0f, 0.0f };
        VkClearValue depth_value{};
        depth_value.depthStencil.depth = 1.0f; // max
        VkClearValue clear_values[2] = { color_value, depth_value };
        VkRenderPassBeginInfo rp_begin_info = vkinit::renderpass_begin_info(_render_pass, _window_extent, _framebuffers[_swapchain_image_index]);
        rp_begin_info.clearValueCount = 2;
        rp_begin_info.pClearValues = clear_values;

        vkCmdBeginRenderPass(cmd, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

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

        vkCmdEndRenderPass(cmd);
    }

    // layout transition
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcQueueFamilyIndex = _graphics_queue_family;
    barrier.dstQueueFamilyIndex = _graphics_queue_family;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.image = _color_attachment[_swapchain_image_index]._image._image;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // pass 1
    {
        VkClearValue color_value{};
        color_value.color = { 0.0f, 0.0f, 0.0f, 0.0f };
        VkRenderPassBeginInfo rp_begin_info = vkinit::renderpass_begin_info(_render_pass1, _window_extent, _framebuffers_pass1[_swapchain_image_index]);
        rp_begin_info.clearValueCount = 1;
        rp_begin_info.pClearValues = &color_value;

        vkCmdBeginRenderPass(cmd, &rp_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_pass1);
        VkPipelineLayout& layout = _pipeline_layout_pass1;

        // for convenience, we use the same uniform buffer
        // uniform buffer (dynamic descriptors)
        uint32_t uniform_offset = padding_size * frame_idx;
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &frame._uniform_data_descriptor_set, 1, &uniform_offset);
        // texture
        VkDescriptorSet& desc_set = _descriptor_set_pass1[_swapchain_image_index];
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline_layout_pass1, 1, 1, &desc_set, 0, nullptr);

        vkCmdDraw(cmd, 3, 1, 0, 0);
        // GUI (at the end of pass 1)
        draw_ui();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

        vkCmdEndRenderPass(cmd);
    }
}

void CAApp::init_descriptors() {
    _descriptors.init_pool(_device);

    using namespace CAAppTest;

    // [1] uniform data
    init_descriptors_for_uniform_data(sizeof(GPUUniformData));
    _set_layout_pass1.push_back(_set_layout.back()); // add to pass 1

    // [2] object data
    init_descriptors_for_object_data(sizeof(GPUObjectData));

    // [3] textures + color attachment
    init_descriptors_for_texture();

    _main_deletion_queue.push_function([&]() { _descriptors.destroy(); });
}

void CAApp::init_descriptors_for_texture() {
    {
        // pass 0
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

    {
        // pass 1
        const uint32_t SETS_NUM = 1;
        VkDescriptorType types[SETS_NUM] = {
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        };
        VkShaderStageFlags stages[SETS_NUM] = {
            VK_SHADER_STAGE_FRAGMENT_BIT,
        };
        VkDescriptorSetLayout pass1_tex_set_layout = _descriptors.create_set_layout(types, stages, SETS_NUM);

        const uint32_t size = _swapchain_images.size();
        _descriptor_set_pass1.resize(size);

        VkSampler tex_sampler = VK_NULL_HANDLE;
        VkSamplerCreateInfo sampler_info = vkinit::sampler_create_info(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
        VK_CHECK(vkCreateSampler(_device, &sampler_info, nullptr, &tex_sampler));

        for (int i = 0; i < size; ++i) {
            // [1] alloc
            VkDescriptorSet desc_set = _descriptors.create_set(pass1_tex_set_layout);;
            _descriptor_set_pass1[i] = desc_set;

            // [2] bind
            VkDescriptorImageInfo attach_info = {};
            attach_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attach_info.imageView = _color_attachment[i]._image_view;
            attach_info.sampler = tex_sampler;
            _descriptors.bind(desc_set, &attach_info, types[0], 0);
        }

        _set_layout_pass1.push_back(pass1_tex_set_layout);

        _main_deletion_queue.push_function(
            [=]() {
                vkDestroySampler(_device, tex_sampler, nullptr);
            }
        );
    }
}

void CAApp::draw_ui() {
    ImGui::Text("FPS: %.2f", fps());
    int id = 0;
    if (ImGui::CollapsingHeader("Camera")) {
        ++id;
        ImGui::PushID(id);
        _camera.draw_ui();
        ImGui::PopID();
    }
    if (ImGui::CollapsingHeader("Chromatic Aberration")) {
        ++id;
        ImGui::PushID(id);
        ImGui::SliderFloat("Strength", &_CA_strength, 0.0f, 0.05f);
        ImGui::PopID();
    }
}