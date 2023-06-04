#include "sbtHelper.h"
#include <cassert>
#include <numeric>

#include "../utils.h"
#include "rtHelper.h"

SBTHelper::SBTHelper() {
    _shader_handle_size = 0u;
    _shader_group_alignment = 0u;
    _num_hit_groups = 0u;
    _num_miss_groups = 0u;
}

void SBTHelper::initialize(
    const uint32_t num_hit_groups,
    const uint32_t num_miss_groups,
    const uint32_t shader_handle_size,
    const uint32_t shader_group_alignment
) {
    _shader_handle_size = shader_handle_size;
    _shader_group_alignment = shader_group_alignment;
    _num_hit_groups = num_hit_groups;
    _num_miss_groups = num_miss_groups;

    _num_hit_shaders.resize(_num_hit_groups, 0u);
    _num_miss_shaders.resize(_num_miss_groups, 0u);

    _stages.clear();
    _groups.clear();
}

void SBTHelper::set_raygen_stage(const VkPipelineShaderStageCreateInfo& stage) {
    // this shader stage should go first!
    assert(_stages.empty());
    _stages.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR info = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, nullptr };
    info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    info.generalShader = 0;
    info.closestHitShader = VK_SHADER_UNUSED_KHR;
    info.anyHitShader = VK_SHADER_UNUSED_KHR;
    info.intersectionShader = VK_SHADER_UNUSED_KHR;
    _groups.push_back(info);
}

void SBTHelper::add_stage_to_hit_groups(const std::vector<VkPipelineShaderStageCreateInfo>& stages, const uint32_t group_index) {
    // raygen stage should go first!
    assert(!_stages.empty());
    // as initialized
    assert(group_index < _num_hit_shaders.size());
    // only 3 hit shaders per group (intersection, any-hit and closest-hit)
    assert(!stages.empty() && stages.size() <= 3);

    // start from 1: raygen is 0
    uint32_t offset = std::accumulate(_num_hit_shaders.begin(), _num_hit_shaders.begin() + group_index, 1);

    // put hit shaders together
    auto it_stage = _stages.begin() + offset;
    _stages.insert(it_stage, stages.begin(), stages.end());

    // create group
    VkRayTracingShaderGroupCreateInfoKHR group_info = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, nullptr };
    group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR; // triangle list
    group_info.generalShader = VK_SHADER_UNUSED_KHR;
    group_info.closestHitShader = VK_SHADER_UNUSED_KHR;
    group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
    group_info.intersectionShader = VK_SHADER_UNUSED_KHR;

    // check all shaders in this group
    for (size_t i = 0; i < stages.size(); ++i) {
        const VkPipelineShaderStageCreateInfo& stage_info = stages[i];
        const uint32_t shader_index = static_cast<uint32_t>(offset + i);
        if (stage_info.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) {
            group_info.closestHitShader = shader_index;
        } else if (stage_info.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR) {
            group_info.anyHitShader = shader_index;
        } else if (stage_info.stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR) {
            group_info.intersectionShader = shader_index;
        }
    }

    // add 1: raygen is 0
    _groups.insert(_groups.begin() + group_index + 1, group_info);
    _num_hit_shaders[group_index] += static_cast<uint32_t>(stages.size());
}

void SBTHelper::add_stage_to_miss_groups(const VkPipelineShaderStageCreateInfo& stage, const uint32_t group_index) {
    // raygen stage should go first!
    assert(!_stages.empty());
    // as initialized
    assert(group_index < _num_miss_shaders.size());

    uint32_t offset = 1;
    offset = std::accumulate(_num_hit_shaders.begin(), _num_hit_shaders.end(), offset);
    offset = std::accumulate(_num_miss_shaders.begin(), _num_miss_shaders.begin() + group_index, offset);

    // put miss shaders together
    // TODO: equals to end()?
    _stages.insert(_stages.begin() + offset, stage);

    // create group
    VkRayTracingShaderGroupCreateInfoKHR group_info = { VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, nullptr };
    group_info.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group_info.generalShader = offset;
    group_info.closestHitShader = VK_SHADER_UNUSED_KHR;
    group_info.anyHitShader = VK_SHADER_UNUSED_KHR;
    group_info.intersectionShader = VK_SHADER_UNUSED_KHR;
    // add it after raygen/hit
    _groups.insert(_groups.begin() + group_index + 1 + _num_hit_groups, group_info);
    ++_num_miss_shaders[group_index];
}

uint32_t SBTHelper::get_num_stages() const {
    return static_cast<uint32_t>(_stages.size());
}

uint32_t SBTHelper::get_num_groups() const {
    return static_cast<uint32_t>(_groups.size());
}

const VkPipelineShaderStageCreateInfo* SBTHelper::get_stages() const {
    return _stages.data();
}

const VkRayTracingShaderGroupCreateInfoKHR* SBTHelper::get_groups() const {
    return _groups.data();
}

uint32_t SBTHelper::get_SBT_size() const {
    return get_num_groups() * _shader_group_alignment;
}

void SBTHelper::set_SBT(AllocatedBuffer buffer) {
    _SBT_buffer = buffer;
}

uint32_t SBTHelper::get_shader_handle_size() const {
    return _shader_handle_size;
}

uint32_t SBTHelper::get_shader_group_alignment() const {
    return _shader_group_alignment;
}


VkDeviceAddress SBTHelper::get_SBT_address(VkDevice device) const {
    return rt_utils::get_buffer_device_address(device, _SBT_buffer._buffer).deviceAddress;
}

uint32_t SBTHelper::get_raygen_offset() const {
    return 0;
}

uint32_t SBTHelper::get_groups_stride() const {
    return _shader_group_alignment;
}

uint32_t SBTHelper::get_raygen_size() const {
    return _shader_group_alignment;
}

uint32_t SBTHelper::get_hit_groups_offset() const {
    return get_raygen_offset() + get_raygen_size();
}

uint32_t SBTHelper::get_hit_groups_size() const{
    return _num_hit_groups * _shader_group_alignment;
}

uint32_t SBTHelper::get_miss_groups_offset() const{
    return get_hit_groups_offset() + get_hit_groups_size();
}

uint32_t SBTHelper::get_miss_groups_size() const {
    return _num_miss_groups * _shader_group_alignment;
}