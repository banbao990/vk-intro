#version 450
#extension GL_KHR_vulkan_glsl : enable

// output
layout (location = 0) out vec3 out_color;

void main() {
    // const array of positions for the triangle
    const vec2 positions[3] = vec2[3](
        vec2( 1.0f,  1.0f),
        vec2(-1.0f,  1.0f),
        vec2( 0.0f, -1.0f)
    );

    // const array of colors for the triangle
    const vec3 colors[3] = vec3[3](
        vec3(1.0f, 0.0f, 0.0f), // red
        vec3(0.0f, 1.0f, 0.0f), // green
        vec3(0.0f, 0.0f, 1.0f)  // blue
    );
    // output the position of each vertex
    int index = gl_VertexIndex % 3;
    int tri_index = gl_VertexIndex / 3;
    vec2 pos = positions[index];
    pos *= 0.2f;
    pos.x += (tri_index - 2) * 0.2f;
    float z = 0.2f * tri_index; // near to far
    gl_Position = vec4(pos, z, 1.0f);
    out_color = colors[index];
}