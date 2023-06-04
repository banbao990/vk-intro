#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

// for run command immediately
struct ImmediateStructure {
    VkFence _upload_fence = VK_NULL_HANDLE;
    VkCommandPool _command_pool = VK_NULL_HANDLE;
    VkCommandBuffer _command_buffer = VK_NULL_HANDLE;
};

struct AllocatedBuffer {
    VkBuffer _buffer = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
    VkDeviceSize _size = 0;
};

struct AllocatedImage {
    VkImage _image = VK_NULL_HANDLE;
    VmaAllocation _allocation = VK_NULL_HANDLE;
};

// depth | color attachment
struct FrameBufferAttachment {
    VkFormat _format = VK_FORMAT_UNDEFINED;
    AllocatedImage _image{};
    VkImageView _image_view = VK_NULL_HANDLE;
};
