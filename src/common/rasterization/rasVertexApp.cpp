#include "rasVertexApp.h"
#include "../initializers.h"
#include "../mesh.h"
#include "../shader.h"

RasVertexApp::~RasVertexApp() {
    // must do it!
    // the resources of this derived class may be freed when the base class call it
    basic_clean_up();
}

void RasVertexApp::init_commands() {
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

void RasVertexApp::init_sync_structures() {
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

void RasVertexApp::add_pipeline(
    const char* vertex_shader_relative_path, const char* fragment_shader_relative_path,
    VkDescriptorSetLayout* set_layout, uint32_t set_layout_count,
    bool use_z_buffer,
    VkRenderPass render_pass,
    uint32_t subpass,
    VkPipeline& pipeline, VkPipelineLayout& layout
) {
    // build the pipeline
    PipelineBuilder pipeline_builder;

    // set input
    // [1] vertex buffer
    pipeline_builder._vertex_input_info = vkinit::pipeline_vertex_input_state_create_info();
    VertexInputDescription vertex_desc = Vertex::get_vertex_description();
    pipeline_builder._vertex_input_info.pVertexAttributeDescriptions = vertex_desc.attributes.data();
    pipeline_builder._vertex_input_info.vertexAttributeDescriptionCount = (uint32_t)vertex_desc.attributes.size();

    pipeline_builder._vertex_input_info.pVertexBindingDescriptions = vertex_desc.bindings.data();
    pipeline_builder._vertex_input_info.vertexBindingDescriptionCount = (uint32_t)vertex_desc.bindings.size();
    // drawing triangle lists, strips, or individual points
    pipeline_builder._input_assembly = vkinit::pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    // [2] layout (buffers), push_constant
    set_shader_input(pipeline_builder, layout, set_layout, set_layout_count);

    // viewport and scissor
    pipeline_builder._viewport.x = 0.0f;
    pipeline_builder._viewport.y = 0.0f;
    pipeline_builder._viewport.width = (float)_window_extent.width;
    pipeline_builder._viewport.height = (float)_window_extent.height;
    pipeline_builder._viewport.minDepth = 0.0f;
    pipeline_builder._viewport.maxDepth = 1.0f;

    pipeline_builder._scissor.offset = { 0,0 };
    pipeline_builder._scissor.extent = _window_extent;

    // wireframe or solid drawing
    pipeline_builder._rasterizer = vkinit::pipeline_rasterization_state_create_info(VK_POLYGON_MODE_FILL);

    // no MSAA
    pipeline_builder._multisampling = vkinit::pipeline_multisampling_state_create_info();

    // no blending
    pipeline_builder._color_blend_attachment = vkinit::pipeline_color_blend_attachment_state();

    // z-buffer
    if (use_z_buffer) {
        pipeline_builder._depth_stencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);
    }

    Shader shader(_device, vertex_shader_relative_path, fragment_shader_relative_path);
    pipeline_builder.reset_shaders();
    pipeline_builder.add_shaders(VK_SHADER_STAGE_VERTEX_BIT, shader.vertex());
    pipeline_builder.add_shaders(VK_SHADER_STAGE_FRAGMENT_BIT, shader.fragment());

    pipeline = pipeline_builder.build_pipeline(_device, _render_pass, use_z_buffer, subpass);

    // cleanup

    _main_deletion_queue.push_function(
        [=]() {
            vkDestroyPipeline(_device, pipeline, nullptr);
        }
    );
    if (layout != VK_NULL_HANDLE) {
        _main_deletion_queue.push_function(
            [=]() {
                vkDestroyPipelineLayout(_device, layout, nullptr);
            }
        );
    }
}

AllocatedBuffer RasVertexApp::create_buffer(uint32_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage) {
    VkBufferCreateInfo buffer_info = vkinit::buffer_create_info(alloc_size, usage);

    VmaAllocationCreateInfo vma_create_info = {};
    vma_create_info.usage = memory_usage;

    AllocatedBuffer buffer{};

    VK_CHECK(vmaCreateBuffer(_allocator, &buffer_info, &vma_create_info, &buffer._buffer, &buffer._allocation, nullptr));
    return buffer;
}

void RasVertexApp::upload_mesh(Mesh& mesh) {
    // CPU Buffer ( for staging )
    const uint32_t buffer_size = (uint32_t)mesh._vertices.size() * sizeof(Vertex);
    AllocatedBuffer staging_buffer = create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // copy to CPU buffer
    void* data;
    vmaMapMemory(_allocator, staging_buffer._allocation, &data);
    memcpy(data, mesh._vertices.data(), buffer_size);
    vmaUnmapMemory(_allocator, staging_buffer._allocation);

    // GPU buffer
    // creates a new VkBuffer, allocates and binds memory for it.
    mesh._vertex_buffer = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // do transfer immediately
    immediate_submit(
        [=](VkCommandBuffer cmd) {
            VkBufferCopy copy = {};
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = buffer_size;
            vkCmdCopyBuffer(cmd, staging_buffer._buffer, mesh._vertex_buffer._buffer, 1, &copy);
        }
    );

    vmaDestroyBuffer(_allocator, staging_buffer._buffer, staging_buffer._allocation);

    _main_deletion_queue.push_function(
        [=]() {
            vmaDestroyBuffer(_allocator, mesh._vertex_buffer._buffer, mesh._vertex_buffer._allocation);
        }
    );
}

void RasVertexApp::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
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
