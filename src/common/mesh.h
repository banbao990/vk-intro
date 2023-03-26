#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "types.h"

class VertexInputDescription {
public:
    std::vector<VkVertexInputBindingDescription> bindings{};
    std::vector<VkVertexInputAttributeDescription> attributes{};
    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    static VertexInputDescription get_vertex_description(
        int binding = 0,
        VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX
    );
};

class Mesh {
public:
    bool load_from_obj(const char* relative_path);

    std::vector<Vertex> _vertices;
    AllocatedBuffer _vertex_buffer = {};
};