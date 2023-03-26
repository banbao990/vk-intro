#include "descriptor.h"
#include "initializers.h"
#include "app.h"

#include <vector>

Descriptor::Descriptor() {
    _free_space = (MAX_SIZE * _types.size());
}

void Descriptor::init_pool(VkDevice device) {
    _device = device;
    const int type_num = _types.size();
    _count.resize(type_num, 0);

    std::vector<VkDescriptorPoolSize> sizes{};
    for (auto t : _types) {
        sizes.push_back({ t, MAX_SIZE });
    }

    VkDescriptorPoolCreateInfo pool_info = vkinit::descriptor_pool_create_info();
    pool_info.flags = 0; // only set FREE_BIT, can call the vkFreeDescritorSet (free set individually)
    pool_info.maxSets = MAX_SIZE * type_num;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();
    VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &_descriptor_pool));
}

void Descriptor::destroy() {
    for (VkDescriptorSetLayout& layout : _layouts) {
        vkDestroyDescriptorSetLayout(_device, layout, nullptr);
    }
    vkDestroyDescriptorPool(_device, _descriptor_pool, nullptr);
}

VkDescriptorSetLayout Descriptor::create_set_layout(VkDescriptorType* type, VkShaderStageFlags* stage_flags, uint32_t binding_num) {
    {
        // check size
        std::vector<int> count_clone = _count;
        for (int i = 0; i < binding_num; ++i) {
            uint32_t index = std::find(_types.begin(), _types.end(), *type) - _types.begin();
            if (index == _types.size()) {
                std::cerr << "[Desctiptor Set] VkDescriptorType not found: " << *type << std::endl;
                return VK_NULL_HANDLE;
            }
            if (++count_clone[index] >= MAX_SIZE) {
                std::cerr << "[Desctiptor Set] No free : " << *type << std::endl;
                return VK_NULL_HANDLE;
            }
        }
        _count = count_clone;
    }

    VkDescriptorSetLayoutCreateInfo info = vkinit::descriptor_set_layout_create_info();
    info.bindingCount =  binding_num;

    std::vector<VkDescriptorSetLayoutBinding> bindings{};
    for (int i = 0; i < binding_num; ++i) {
        bindings.push_back(vkinit::descriptor_set_layout_binding(type[i], stage_flags[i], i));
    }

    info.pBindings = bindings.data();
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorSetLayout(_device, &info, nullptr, &layout));
    _layouts.push_back(layout);
    return layout;
}

VkDescriptorSet Descriptor::create_set(VkDescriptorSetLayout layout) {
    if (--_free_space < 0) {
        std::cerr << "[Desctiptor Set] No free Set!" << std::endl;
        return VK_NULL_HANDLE;
    }
    VkDescriptorSetAllocateInfo info = vkinit::descriptor_set_allocate_info();
    info.descriptorPool = _descriptor_pool;
    info.descriptorSetCount = 1;
    info.pSetLayouts = &layout;

    VkDescriptorSet descriptor_set;
    VK_CHECK(vkAllocateDescriptorSets(_device, &info, &descriptor_set));
    return descriptor_set;
}

void Descriptor::bind(VkDescriptorSet descriptor_set, VkDescriptorBufferInfo* info, VkDescriptorType type, int binding) {
    uint32_t index = std::find(_types.begin(), _types.end(), type) - _types.begin();
    if (index == _types.size()) {
        std::cerr << "[Desctiptor Set] VkDescriptorType not found: " << type << std::endl;
        return;
    }
    if (_count[index] >= MAX_SIZE) {
        std::cerr << "[Desctiptor Set] No free : " << type << std::endl;
        return;
    }

    VkWriteDescriptorSet write_set = vkinit::write_descriptor_buffer(
        type, descriptor_set, info, binding
    );

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
}

void Descriptor::bind(VkDescriptorSet descriptor_set, VkDescriptorImageInfo* info, VkDescriptorType type, int binding) {
    VkWriteDescriptorSet write_set = vkinit::write_descriptor_image(
        type, descriptor_set, info, binding
    );

    vkUpdateDescriptorSets(_device, 1, &write_set, 0, nullptr);
}
