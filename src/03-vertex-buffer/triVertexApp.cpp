#include "triVertexApp.h"
#include "../common/initializers.h"

TriVertexApp::~TriVertexApp() {
    basic_clean_up();
}

void TriVertexApp::init_pipeline() {
    add_pipeline("03-mesh.vert.spv", "03-mesh.frag.spv", nullptr, 0, true, _render_pass, 0, _tri_pipeline, _tri_pipeline_layout);
}

void TriVertexApp::init_scenes() {
    _tri_mesh._vertices.resize(3);

    _tri_mesh._vertices[0].position = 0.2f * glm::vec3(1.0f, 1.0f, 0.0f);
    _tri_mesh._vertices[1].position = 0.2f * glm::vec3(-1.0f, 1.0f, 0.0f);
    _tri_mesh._vertices[2].position = 0.2f * glm::vec3(0.0f, -1.0f, 0.0f);

    // use normal channel as color channel
    _tri_mesh._vertices[0].normal = { 1.0f, 0.0f, 0.0f }; // red
    _tri_mesh._vertices[1].normal = { 0.0f, 1.0f, 0.0f }; // green
    _tri_mesh._vertices[2].normal = { 0.0f, 0.0f, 1.0f }; // blue

    //_tri_mesh.load_from_obj("triangle.obj");
    upload_mesh(_tri_mesh);
}

void TriVertexApp::render() {
    VkCommandBuffer& cmd = get_current_frame()._main_command_buffer;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _tri_pipeline);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &_tri_mesh._vertex_buffer._buffer, &offset);
    vkCmdDraw(cmd, _tri_mesh._vertices.size(), 1, 0, 0);
}