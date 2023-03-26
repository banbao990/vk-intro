#version 460
// input
layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;

// output
layout (location = 0) out vec2 out_uv;

struct CameraBuffer {
    mat4 _view;
    mat4 _proj;
    mat4 _view_proj; // P * V
};

struct ObjectData {
    mat4 _model;
};

// uniform buffer
layout (set = 0, binding = 0) uniform UniformData {
    CameraBuffer _camera;
} uniform_data;

// storage buffer
layout (std140, set = 1, binding = 0) readonly buffer ObjectBuffer {
    ObjectData _objects[];
} object_data;

void main() {
    mat4 model = object_data._objects[gl_BaseInstance]._model; // >= 460
    mat4 MVP = uniform_data._camera._view_proj * model;
    gl_Position = MVP * vec4(in_position, 1.0f);
    out_uv = in_uv;
}