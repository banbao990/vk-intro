#include "rtHelper.h"
#include "common.h"
#include "../initializers.h"
#include "../utils.h"
#include "rt.h"

LoaderManager* LoaderManager::instance = nullptr;

LoaderManager::LoaderManager() {}

LoaderManager::~LoaderManager() {
    if (instance != nullptr) {
        delete instance;
    }
}

LoaderManager* LoaderManager::get_instance() { 
    if (instance == nullptr) {
        instance = new LoaderManager();
    }
    return instance; 
}

void LoaderManager::init_extension_addr(const VkDevice device) {
    LoaderManager* instance = LoaderManager::get_instance();
    instance->vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
    instance->vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
    instance->vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
    instance->vkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
    instance->vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
    instance->vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
    instance->vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
    instance->vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
    instance->vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
}

AllocatedBuffer rt_utils::create_buffer(const VmaAllocator& allocator,
    const uint32_t alloc_size, VkBufferUsageFlags usage, VmaMemoryUsage memory_usage
) {
    assert(alloc_size != 0);
    VkBufferCreateInfo buffer_info = vkinit::buffer_create_info(alloc_size, usage);

    VmaAllocationCreateInfo vma_create_info = {};
    vma_create_info.usage = memory_usage;

    AllocatedBuffer buffer{};

    VK_CHECK(vmaCreateBuffer(allocator, &buffer_info, &vma_create_info, &buffer._buffer, &buffer._allocation, nullptr));

    buffer._size = alloc_size;
    return buffer;
}

VkDeviceOrHostAddressKHR rt_utils::get_buffer_device_address(const VkDevice& device, const VkBuffer& buffer) {
    VkBufferDeviceAddressInfoKHR info = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        nullptr,
        buffer
    };

    VkDeviceOrHostAddressKHR result;
    result.deviceAddress = LoaderManager::get_instance()->vkGetBufferDeviceAddressKHR(device, &info);

    return result;
}

VkDeviceOrHostAddressConstKHR rt_utils::get_buffer_device_address_const(const VkDevice& device, const VkBuffer& buffer) {
    VkDeviceOrHostAddressKHR address = rt_utils::get_buffer_device_address(device, buffer);

    VkDeviceOrHostAddressConstKHR result;
    result.deviceAddress = address.deviceAddress;

    return result;
}

void RTScene::build_blas(VkDevice device, VmaAllocator allocator, RTApp* app) {
    const size_t num_meshes = _meshes.size();

    std::vector<VkAccelerationStructureGeometryKHR> geometries(num_meshes, VkAccelerationStructureGeometryKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR });
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> ranges(num_meshes, VkAccelerationStructureBuildRangeInfoKHR{});
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> build_infos(num_meshes, VkAccelerationStructureBuildGeometryInfoKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR });
    std::vector<VkAccelerationStructureBuildSizesInfoKHR> size_infos(num_meshes, VkAccelerationStructureBuildSizesInfoKHR{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR });

    for (size_t i = 0; i < num_meshes; ++i) {
        RTMesh& mesh = _meshes[i];

        VkAccelerationStructureGeometryKHR& geometry = geometries[i];
        VkAccelerationStructureBuildRangeInfoKHR& range = ranges[i];
        VkAccelerationStructureBuildGeometryInfoKHR& build_info = build_infos[i];
        VkAccelerationStructureBuildSizesInfoKHR& size_info = size_infos[i];

        range.primitiveCount = mesh._num_faces;

        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        
        VkAccelerationStructureGeometryTrianglesDataKHR& triangle = geometry.geometry.triangles;
        triangle.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        triangle.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        triangle.vertexData = rt_utils::get_buffer_device_address_const(device, mesh._positions._buffer);// device or host address
        triangle.vertexStride = sizeof(vec3);
        triangle.maxVertex = mesh._num_vertices;
        triangle.indexData = rt_utils::get_buffer_device_address_const(device, mesh._indices._buffer);
        triangle.indexType = VK_INDEX_TYPE_UINT32;

        build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        build_info.geometryCount = 1;
        build_info.pGeometries = &geometry;

        // Retrieve the required size for an acceleration structure
        LoaderManager::get_instance()->vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &range.primitiveCount, &size_info);
    }

    VkDeviceSize max_blas_size = 0;
    for (const auto& size_info : size_infos) {
        max_blas_size = std::max(size_info.buildScratchSize, max_blas_size);
    }
    max_blas_size += app->get_min_acceleration_structure_scratch_offset_alignment();

    // just for staging, need it to build blas
    AllocatedBuffer scratch_buffer = rt_utils::create_buffer(allocator, max_blas_size,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    LoaderManager* loader_manager = LoaderManager::get_instance();

    app->immediate_submit(
        [&](VkCommandBuffer cmd) {
            VkMemoryBarrier memory_barrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };
            memory_barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            memory_barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

            for (size_t i = 0; i < num_meshes; ++i) {
                RTMesh& mesh = _meshes[i];
                VkAccelerationStructureBuildSizesInfoKHR& size_info = size_infos[i];
                VkAccelerationStructureBuildGeometryInfoKHR& build_info = build_infos[i];
                const VkAccelerationStructureBuildRangeInfoKHR* range[1] = { &ranges[i] };

                mesh._blas._buffer = rt_utils::create_buffer(allocator, size_info.accelerationStructureSize,
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    VMA_MEMORY_USAGE_GPU_ONLY
                );

                VkAccelerationStructureCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
                create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                create_info.size = size_info.accelerationStructureSize;
                create_info.buffer = mesh._blas._buffer._buffer;

                loader_manager->vkCreateAccelerationStructureKHR(device, &create_info, nullptr, &mesh._blas._acceleration_structure);

                // aligned VkPhysicalDeviceAccelerationStructurePropertiesKHR::minAccelerationStructureScratchOffsetAlignment
                build_info.scratchData = rt_utils::get_buffer_device_address(device, scratch_buffer._buffer);
                build_info.scratchData.deviceAddress = vkutils::padding(build_info.scratchData.deviceAddress, app->get_min_acceleration_structure_scratch_offset_alignment());

                build_info.srcAccelerationStructure = VK_NULL_HANDLE;
                build_info.dstAccelerationStructure = mesh._blas._acceleration_structure;

                loader_manager->vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, range);

                // guard our scratch buffer
                vkCmdPipelineBarrier(cmd,
                    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                    0, 1, &memory_barrier, 0, nullptr, 0, nullptr
                );
            }
        }
    );

    vmaDestroyBuffer(allocator, scratch_buffer._buffer, scratch_buffer._allocation);

    // get handles(for tlas)
    for (size_t i = 0; i < num_meshes; ++i) {
        RTMesh& mesh = _meshes[i];
        VkAccelerationStructureDeviceAddressInfoKHR address_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        address_info.accelerationStructure = mesh._blas._acceleration_structure;
        mesh._blas._handle = loader_manager->vkGetAccelerationStructureDeviceAddressKHR(device, &address_info);
    }

    app->add_to_deletion_queue(
        [=]() {
            for (size_t i = 0; i < num_meshes; ++i) {
                RTMesh& mesh = _meshes[i];
                vmaDestroyBuffer(allocator, mesh._blas._buffer._buffer, mesh._blas._buffer._allocation);
                loader_manager->vkDestroyAccelerationStructureKHR(device, mesh._blas._acceleration_structure, nullptr);
            }
        }
    );
}

void RTScene::build_tlas(VkDevice device, VmaAllocator allocator, RTApp* app) {
    LoaderManager* loader_manager = LoaderManager::get_instance();

    // now no transform
    const VkTransformMatrixKHR transform = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };

    const size_t num_meshes = _meshes.size();

    // create instances for our meshes
    std::vector<VkAccelerationStructureInstanceKHR> instances(num_meshes, VkAccelerationStructureInstanceKHR{});

    for (size_t i = 0; i < num_meshes; ++i) {
        RTMesh& mesh = _meshes[i];
        VkAccelerationStructureInstanceKHR& instance = instances[i];

        instance.transform = transform;
        instance.instanceCustomIndex = static_cast<uint32_t>(i);
        instance.mask = 0xff;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = mesh._blas._handle;
    }

    const uint32_t instance_size = static_cast<uint32_t>(instances.size() * sizeof(VkAccelerationStructureInstanceKHR));

    // CPU Buffer ( for staging )
    AllocatedBuffer staging_buffer = rt_utils::create_buffer(allocator, instance_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

    // copy to CPU buffer
    void* data;
    vmaMapMemory(allocator, staging_buffer._allocation, &data);
    memcpy(data, instances.data(), instance_size);
    vmaUnmapMemory(allocator, staging_buffer._allocation);

    AllocatedBuffer instance_buffer = rt_utils::create_buffer(allocator, instance_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    // transfer to GPU
    app->immediate_submit(
        [=](VkCommandBuffer cmd) {
            VkBufferCopy copy = {};
            copy.srcOffset = 0;
            copy.dstOffset = 0;
            copy.size = instance_size;
            vkCmdCopyBuffer(cmd, staging_buffer._buffer, instance_buffer._buffer, 1, &copy);
        }
    );

    // create tlas
    VkAccelerationStructureGeometryInstancesDataKHR tlas_instance_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
    tlas_instance_info.data = rt_utils::get_buffer_device_address_const(device, instance_buffer._buffer);

    VkAccelerationStructureGeometryKHR tlas_geo_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    tlas_geo_info.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    tlas_geo_info.geometry.instances = tlas_instance_info;

    VkAccelerationStructureBuildGeometryInfoKHR build_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
    build_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    build_info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    build_info.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    build_info.geometryCount = 1;
    build_info.pGeometries = &tlas_geo_info;

    const uint32_t num_instances = static_cast<uint32_t>(instances.size());
    
    VkAccelerationStructureBuildSizesInfoKHR size_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    loader_manager->vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, &num_instances, &size_info);
    _tlas._buffer = rt_utils::create_buffer(allocator, size_info.accelerationStructureSize, 
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    VkAccelerationStructureCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
    create_info.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    create_info.size = size_info.accelerationStructureSize;
    create_info.buffer = _tlas._buffer._buffer;

    VK_CHECK(loader_manager->vkCreateAccelerationStructureKHR(device, &create_info, nullptr, &_tlas._acceleration_structure));

    // just for staging, need it to build tlas
    AllocatedBuffer scratch_buffer = rt_utils::create_buffer(allocator, size_info.accelerationStructureSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY
    );

    build_info.scratchData = rt_utils::get_buffer_device_address(device, scratch_buffer._buffer);
    build_info.srcAccelerationStructure = VK_NULL_HANDLE;
    build_info.dstAccelerationStructure = _tlas._acceleration_structure;

    VkAccelerationStructureBuildRangeInfoKHR range = {};
    range.primitiveCount = num_instances;
    const VkAccelerationStructureBuildRangeInfoKHR* ranges[1] = { &range };

    app->immediate_submit(
        [&](VkCommandBuffer cmd) {
            loader_manager->vkCmdBuildAccelerationStructuresKHR(cmd, 1, &build_info, ranges);
        }
    );

    // get_handle
    VkAccelerationStructureDeviceAddressInfoKHR address_info = {};
    address_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    address_info.accelerationStructure = _tlas._acceleration_structure;
    _tlas._handle = loader_manager->vkGetAccelerationStructureDeviceAddressKHR(device, &address_info);

    vmaDestroyBuffer(allocator, scratch_buffer._buffer, scratch_buffer._allocation);
    vmaDestroyBuffer(allocator, instance_buffer._buffer, instance_buffer._allocation);
    vmaDestroyBuffer(allocator, staging_buffer._buffer, staging_buffer._allocation);

    app->add_to_deletion_queue(
        [=]() {
            vmaDestroyBuffer(allocator, _tlas._buffer._buffer, _tlas._buffer._allocation);
            loader_manager->vkDestroyAccelerationStructureKHR(device, _tlas._acceleration_structure, nullptr);
        }
    );
}

void rt_utils::image_barrier(VkCommandBuffer cmd,
    VkImage image,
    VkImageSubresourceRange& subresource_range,
    VkAccessFlags src_access_mask,
    VkAccessFlags dst_access_mask,
    VkImageLayout old_layout,
    VkImageLayout new_layout) {

    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext = nullptr;
    imageMemoryBarrier.srcAccessMask = src_access_mask;
    imageMemoryBarrier.dstAccessMask = dst_access_mask;
    imageMemoryBarrier.oldLayout = old_layout;
    imageMemoryBarrier.newLayout = new_layout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresource_range;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr, 1,
        &imageMemoryBarrier);
}