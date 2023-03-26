#version 450

#extension GL_KHR_vulkan_glsl : enable


struct CameraBuffer {
    mat4 _view;
    mat4 _proj;
    mat4 _view_proj; // P * V
};

// uniform buffer
layout (set = 0, binding = 0) uniform UniformData {
    CameraBuffer _camera;
    float _CA_strength;
} uniform_data;

layout (set = 1, binding = 0) uniform sampler2D input_color;

layout (location = 0) in vec2 in_uv;
layout (location = 0) out vec4 out_color;

void main() {
    vec2 uv_01 = in_uv * 0.5f + 0.5f;
    out_color = vec4(
        texture(input_color, uv_01 + vec2(uniform_data._CA_strength, 0.0)).r,
        texture(input_color, uv_01).g,
        texture(input_color, uv_01 + vec2(-uniform_data._CA_strength, 0.0)).b,
        1.0
    );
}