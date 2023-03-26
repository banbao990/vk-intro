#version 450
#extension GL_KHR_vulkan_glsl : enable 

// input
// if there exsits mismatch between shader stages, VkPipiline compilation will fail
layout (location = 0) in vec2 in_uv;

// output
layout (location = 0) out vec4 out_frag_color;

layout (set = 2, binding = 0) uniform sampler2D tex_sampler;

void main() {
    vec3 color = texture(tex_sampler, in_uv).rgb;
    out_frag_color = vec4(color, 1.0f);
}