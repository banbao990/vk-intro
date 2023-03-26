#version 450

#extension GL_KHR_vulkan_glsl : enable

// output
layout (location = 0) out vec2 out_uv;

void main() {
    // const array of positions for the triangle
    const vec2 positions[3] = vec2[3](
        vec2(-1.0f, -1.0f),
        vec2( 3.0f, -1.0f),
        vec2(-1.0f,  3.0f)
    );
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.5, 1.0);
    out_uv = pos;
}
