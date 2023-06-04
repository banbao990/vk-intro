#pragma once
#include "../types.h"
#include <vector>

struct RTAccelerationStructure {
    AllocatedBuffer                         _buffer;
    VkAccelerationStructureKHR              _acceleration_structure;
    VkDeviceAddress                         _handle;
};

struct RTMesh {
    uint32_t                    _num_vertices;
    uint32_t                    _num_faces;

    AllocatedBuffer             _positions;
    AllocatedBuffer             _attribs;
    AllocatedBuffer             _indices;
    AllocatedBuffer             _faces;
    AllocatedBuffer             _mat_IDs;

    RTAccelerationStructure     _blas;
};

struct RTMaterial {
    VkFormat            _format;
    VkImageView         _image_view;
    VkSampler           _sampler;
    AllocatedImage      _texture;
};

// forward declaration
class RTApp;

struct RTScene {
    std::vector<RTMesh>             _meshes;
    std::vector<RTMaterial>         _materials;
    RTAccelerationStructure         _tlas;

    // shader resources stuff
    std::vector<VkDescriptorBufferInfo>   _mat_IDs_buffer_infos;
    std::vector<VkDescriptorBufferInfo>   _attribs_buffer_infos;
    std::vector<VkDescriptorBufferInfo>   _faces_buffer_infos;
    std::vector<VkDescriptorImageInfo>    _textures_infos;

    void build_blas(VkDevice device, VmaAllocator allocator, RTApp* app);
    void build_tlas(VkDevice device, VmaAllocator allocator, RTApp* app);
};

// single instance
class LoaderManager {
private:
    static LoaderManager *instance;
    LoaderManager();
    ~LoaderManager();

public:
    static LoaderManager* get_instance();
    void init_extension_addr(const VkDevice device);

    // loader
    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR{};
    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR{};
    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR{};
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR{};
    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR{};
    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR{};
    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR{};
    PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR{};
    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR{};
};

namespace rt_utils {

    void init_extension_addr(const VkDevice _device);

    [[nodiscard]]
    AllocatedBuffer create_buffer(const VmaAllocator& allocator, const uint32_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage);

    VkDeviceOrHostAddressKHR get_buffer_device_address(const VkDevice& device, const VkBuffer& buffer);
    VkDeviceOrHostAddressConstKHR get_buffer_device_address_const(const VkDevice& device, const VkBuffer& buffer);
    void image_barrier(VkCommandBuffer cmd,
        VkImage image,
        VkImageSubresourceRange& subresource_range,
        VkAccessFlags src_access_mask,
        VkAccessFlags dst_access_mask,
        VkImageLayout old_layout,
        VkImageLayout new_layout);
}