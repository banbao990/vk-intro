#pragma once

#include "config.h"
#include "types.h"

class Shader {
public:
    Shader(VkDevice device, const char* vertex_relative_path, const char* fragment_relative_path);
    virtual ~Shader();

    VkShaderModule vertex() const;
    VkShaderModule fragment() const;

    static VkShaderModule load_shader_module(VkDevice device, const char* file_relative_path);
private:
    VkDevice _device;
    VkShaderModule _vertex_shader = VK_NULL_HANDLE;
    VkShaderModule _fragment_shader = VK_NULL_HANDLE;
};