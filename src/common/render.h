#pragma once

#include "mesh.h"

#include <glm/glm.hpp>

#include <stb_image.h>

struct Texture {
    AllocatedImage _image{};
    VkImageView _image_view = VK_NULL_HANDLE;
};

struct MaterialLayout {
    VkDescriptorSet _texture_set = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
};

class Material {
public:
    stbi_uc* _pixels;
    int _width;
    int _height;
    int _channels;
    MaterialLayout _layout{};
    Texture _texture{};

    void load_image_from_file(const char* file);
    void free();
};

class RenderObject {
public:
    Mesh _mesh{};
    Material _material{};
    glm::mat4 _model_matrix = glm::mat4(1.0f);

    inline VkPipeline& pipeline() { return _material._layout._pipeline; }
    inline VkPipelineLayout& pipeline_layout() { return _material._layout._pipeline_layout; }
    inline AllocatedImage& image() { return _material._texture._image; }
    inline VkImageView& image_view() { return _material._texture._image_view; }
    inline VkDescriptorSet& descriptor_set() { return _material._layout._texture_set; }
};