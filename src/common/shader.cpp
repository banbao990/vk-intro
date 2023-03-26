#include "shader.h"
#include "initializers.h"

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

Shader::Shader(VkDevice device, const char* vertex_relative_path, const char* fragment_relative_path) :_device(device) {
    _vertex_shader = load_shader_module(device, vertex_relative_path);
    _fragment_shader = load_shader_module(device, fragment_relative_path);
}

Shader::~Shader() {
    vkDestroyShaderModule(_device, _vertex_shader, nullptr);
    vkDestroyShaderModule(_device, _fragment_shader, nullptr);
}

VkShaderModule Shader::vertex() const {
    return _vertex_shader;
}

VkShaderModule Shader::fragment() const {
    return _fragment_shader;
}

VkShaderModule Shader::load_shader_module(VkDevice device, const char* file_relative_path) {
    // 1. open the file
    // ate: with the cursor at the end
    std::string file_path = std::string(SHADER_DIRECTORY"/") + file_relative_path;
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Shader Loading] Shadee file not found: " << file_path << std::endl;
        return VK_NULL_HANDLE;
    }

    // 2. copy to the buffer
    // file size in byte
    size_t file_size = (size_t)file.tellg();
    // TODO: why use uint32_t but not char
    std::vector<uint32_t> buffer((file_size + sizeof(uint32_t) - 1) / sizeof(uint32_t));
    // set the curse at the beginning
    file.seekg(0);
    file.read((char*)buffer.data(), file_size);
    file.close();

    // 3. load to the vulkan
    VkShaderModuleCreateInfo create_info = vkinit::shader_module_create_info();

    // codeSize: in byte
    create_info.codeSize = buffer.size() * sizeof(uint32_t);
    create_info.pCode = buffer.data();

    VkShaderModule shader_module;
    VkResult create_result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module);
    if (create_result != VK_SUCCESS) {
        std::cout << "[Shader Loading] Shader File Error: " << create_result << std::endl;
        return VK_NULL_HANDLE;
    }

    return shader_module;
}
