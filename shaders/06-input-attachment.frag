#version 450
#extension GL_KHR_vulkan_glsl : enable

layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput input_color;
layout (input_attachment_index = 1, set = 0, binding = 1) uniform subpassInput input_depth;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

void main()  {
    if (in_uv.x < in_uv.y) {
        vec3 color = subpassLoad(input_color).rgb;
        out_color = vec4(color, 1.0f);
    }
    else {
        float depth = subpassLoad(input_depth).r;
        const float min_depth = 0.95f;
        const float max_depth = 1.0f;
        depth = (clamp(depth, min_depth, max_depth) - min_depth) / (max_depth - min_depth);
        out_color = vec4(depth, depth, depth, 1.0f);
    }
}