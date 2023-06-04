#pragma once

#include <vector>
#include "types.h"


class Descriptor {
public:
    Descriptor();
    void init_pool(VkDevice device);
    void destroy();
    VkDescriptorSetLayout create_set_layout(VkDescriptorType* type, VkShaderStageFlags* stage_flags, uint32_t binding_num = 1,
        uint32_t* descriptor_count = nullptr, VkDescriptorSetLayoutBindingFlagsCreateInfo* binding_flags = nullptr);
    VkDescriptorSet create_set(VkDescriptorSetLayout layout);
    void create_set(VkDescriptorSet* descriptor_set, VkDescriptorSetLayout* layout, uint32_t* descriptors_cnt, uint32_t num);

    void bind(VkDescriptorSet descriptor_set, VkDescriptorBufferInfo* info, VkDescriptorType type, int binding);
    void bind(VkDescriptorSet descriptor_set, VkDescriptorImageInfo* info, VkDescriptorType type, int binding);
    void bind(VkWriteDescriptorSet* write_set, uint32_t count);

private:
    static const int MAX_SIZE = 100;
    VkDevice _device = VK_NULL_HANDLE;
    std::vector<VkDescriptorType> _types;
    int _free_space{0};
    std::vector<int> _count{};
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayout> _layouts{};
};