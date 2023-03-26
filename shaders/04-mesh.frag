#version 450

// input
// if there exsits mismatch between shader stages, VkPipiline compilation will fail
layout (location = 0) in vec3 in_color;

// output
layout (location = 0) out vec4 out_frag_color;

void main() {
    out_frag_color = vec4(in_color, 1.0f);
}