#include "mesh.h"
#include "config.h"

#include <iostream>

#include <tiny_obj_loader.h>

VertexInputDescription Vertex::get_vertex_description(
    int binding, VkVertexInputRate input_rate
) {
    VertexInputDescription description = {};

    VkVertexInputBindingDescription main_binding = {};
    main_binding.binding = binding;
    main_binding.inputRate = input_rate;
    main_binding.stride = sizeof(Vertex);

    description.bindings.push_back(main_binding);

    // position : location 0
    VkVertexInputAttributeDescription position_attribute = {};
    position_attribute.binding = binding;
    position_attribute.location = 0;
    position_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    position_attribute.offset = offsetof(Vertex, position);

    // normal   : location 1
    VkVertexInputAttributeDescription normal_attribute = {};
    normal_attribute.binding = binding;
    normal_attribute.location = 1;
    normal_attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
    normal_attribute.offset = offsetof(Vertex, normal);

    // uv       : location 2
    VkVertexInputAttributeDescription uv_attribute = {};
    uv_attribute.binding = binding;
    uv_attribute.location = 2;
    uv_attribute.format = VK_FORMAT_R32G32_SFLOAT;
    uv_attribute.offset = offsetof(Vertex, uv);

    description.attributes.push_back(position_attribute);
    description.attributes.push_back(normal_attribute);
    description.attributes.push_back(uv_attribute);

    return description;
}

bool Mesh::load_from_obj(const char* relative_path) {
    std::string path = std::string(ASSETS_DIRECTORY"/") + relative_path;
    // 1. tinyobj loading
    tinyobj::attrib_t attrib;                       // vertex
    std::vector<tinyobj::shape_t> shapes;           // objects
    std::vector<tinyobj::material_t> materials;     // materials (do not use)
    std::string warn, err;                          // loading info

    // assume the *.mtl file is in the same dir
    std::string mtl_dir = std::string(path);
    {
        size_t idx = mtl_dir.rfind('/');
        if (idx == std::string::npos) {
            mtl_dir = "";
        } else {
            mtl_dir = mtl_dir.substr(0, idx);
        }
    }

    // trianglulate = true by default
    tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str(), mtl_dir.c_str());
    if (!warn.empty()) {
        std::cout << "[Obj Loading] " << path << ", Warning: " << warn << std::endl;
    }
    if (!err.empty()) {
        std::cout << "[Obj Loading] " << path << ", Error: " << warn << std::endl;
        return false;
    }

    // 2. construct the structure (_vertices, _vertex_buffer)
    _vertices.clear();
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); ++s) {
        // Loop over faces (polygon)
        size_t index_offset = 0;
        size_t vertices_num = shapes[s].mesh.num_face_vertices.size();
        for (size_t f = 0; f < vertices_num; ++f) {
            // hardcode loading to triangles (trianglulate = true)
            const int fv = 3;
            // Loop over vertices in the face.
            const bool have_normal = !attrib.normals.empty();
            const bool have_uv = !attrib.texcoords.empty();
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                // vertex position
                int v_idx = 3 * idx.vertex_index;
                tinyobj::real_t vx = attrib.vertices[v_idx + 0];
                tinyobj::real_t vy = attrib.vertices[v_idx + 1];
                tinyobj::real_t vz = attrib.vertices[v_idx + 2];

                // vertex normal
                tinyobj::real_t nx = 0, ny = 0, nz = 1;
                if (have_normal) {
                    int n_idx = 3 * idx.normal_index;
                    nx = attrib.normals[n_idx + 0];
                    ny = attrib.normals[n_idx + 1];
                    nz = attrib.normals[n_idx + 2];
                }

                // uv
                tinyobj::real_t uv_x = 0, uv_y = 0;
                if (have_uv) {
                    int uv_idx = 2 * idx.texcoord_index;
                    uv_x = attrib.texcoords[uv_idx + 0];
                    uv_y = attrib.texcoords[uv_idx + 1];
                }

                // copy it into our vertex
                Vertex new_vert = {};
                new_vert.position.x = vx;
                new_vert.position.y = vy;
                new_vert.position.z = vz;
                new_vert.normal.x = nx;
                new_vert.normal.y = ny;
                new_vert.normal.z = nz;
                new_vert.uv.x = uv_x;
                new_vert.uv.y = 1.0f - uv_y; // stb

                // we are setting the vertex color as the vertex normal. This is just for display purposes
                _vertices.push_back(new_vert);
            }
            index_offset += fv;
        }
    }

    std::cout << "[Obj Loading] " << relative_path << ", total vertices: " << _vertices.size() << std::endl;
    return true;
}
