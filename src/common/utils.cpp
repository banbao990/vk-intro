#include "utils.h"

uint64_t vkutils::padding(uint64_t original_size, uint64_t alignment) {
    const uint64_t min_align = alignment;
    uint64_t aligned_size = original_size;
    if (min_align > 0) {
        // aligned_size = (aligned_size + min_align - 1) % aligned_size * min_align;
        // note the aligned size must be the power of 2
        aligned_size = (aligned_size + min_align - 1) & (~(min_align - 1));
    }
    return aligned_size;
}

glm::vec3 vkutils::polar_to_cartesian(float yaw, float pitch) {
    float y = cosf(pitch);
    float sinPitch = sinf(pitch);
    float x = sinPitch * cosf(yaw);
    float z = sinPitch * sinf(yaw);
    return glm::vec3(x, y, z);
}