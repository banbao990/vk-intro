#pragma once 

#include <stdint.h>
#include <vector>

#include "../types.h"

// Shader Binding Table
class SBTHelper {
    /* 
     * layout:
     * 
     * | raygen | hit | miss |
     * 
    */

public:
    SBTHelper();
    ~SBTHelper() = default;

    void        initialize(const uint32_t num_hit_groups, const uint32_t num_miss_groups, const uint32_t shader_handle_size, const uint32_t shader_group_alignment);
    void        destroy();
    void        set_raygen_stage(const VkPipelineShaderStageCreateInfo& stage);
    void        add_stage_to_hit_groups(const std::vector<VkPipelineShaderStageCreateInfo>& stages, const uint32_t group_index);
    void        add_stage_to_miss_groups(const VkPipelineShaderStageCreateInfo& stage, const uint32_t group_index);

    uint32_t    get_groups_stride() const;
    uint32_t    get_raygen_offset() const;
    uint32_t    get_raygen_size() const;
    uint32_t    get_hit_groups_offset() const;
    uint32_t    get_hit_groups_size() const;
    uint32_t    get_miss_groups_offset() const;
    uint32_t    get_miss_groups_size() const;

    uint32_t    get_num_stages() const;
    const VkPipelineShaderStageCreateInfo* get_stages() const;
    uint32_t    get_num_groups() const;
    const VkRayTracingShaderGroupCreateInfoKHR* get_groups() const;

    uint32_t get_shader_handle_size() const;

    uint32_t    get_SBT_size() const;
    void        set_SBT(AllocatedBuffer buffer);
    VkDeviceAddress get_SBT_address(VkDevice device) const;

    uint32_t get_shader_group_alignment() const;

private:
    uint32_t                                          _shader_handle_size;
    uint32_t                                          _shader_group_alignment;
    uint32_t                                          _num_hit_groups;
    uint32_t                                          _num_miss_groups;
    std::vector<uint32_t>                             _num_hit_shaders;
    std::vector<uint32_t>                             _num_miss_shaders;
    std::vector<VkPipelineShaderStageCreateInfo>      _stages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> _groups;
    AllocatedBuffer                                   _SBT_buffer;
};
