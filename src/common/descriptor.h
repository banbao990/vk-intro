#pragma once

#include <vector>
#include "types.h"


class Descriptor {
public:
    Descriptor();
    void init_pool(VkDevice device);
    void destroy();
    VkDescriptorSetLayout create_set_layout(VkDescriptorType* type, VkShaderStageFlags* stage_flags, uint32_t binding_num = 1);
    VkDescriptorSet create_set(VkDescriptorSetLayout layout);
    void bind(VkDescriptorSet descriptor_set, VkDescriptorBufferInfo* info, VkDescriptorType type, int binding);
    void bind(VkDescriptorSet descriptor_set, VkDescriptorImageInfo* info, VkDescriptorType type, int binding);

private:
    static const int MAX_SIZE = 10;
    VkDevice _device = VK_NULL_HANDLE;
    std::vector<VkDescriptorType> _types{
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };
    int _free_space{0};
    std::vector<int> _count{};
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> _layouts{};
};