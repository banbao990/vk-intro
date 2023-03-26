#define STB_IMAGE_IMPLEMENTATION
#include "render.h"
#include "config.h"

#include <iostream>
#include <string>

void Material::load_image_from_file(const char* relative_path) {
    std::string path = std::string(ASSETS_DIRECTORY"/") + relative_path;
    stbi_uc* pixels = stbi_load(path.c_str(), &_width, &_height, &_channels, STBI_rgb_alpha);
    if (!pixels) {
        std::cout << "[Image]: Failed to load " << relative_path << std::endl;
    }
    _pixels = pixels;
}

void Material::free() {
    stbi_image_free(_pixels);
}
