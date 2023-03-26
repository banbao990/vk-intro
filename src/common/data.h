#pragma once
#include <types.h>

struct FrameData {
    // sync strcuture
    VkSemaphore _present_semaphore = VK_NULL_HANDLE;
    VkSemaphore _render_semaphore = VK_NULL_HANDLE;
    VkFence _render_fence = VK_NULL_HANDLE;

    // command
    VkCommandPool _command_pool = VK_NULL_HANDLE;          // the command pool for our commands
    VkCommandBuffer _main_command_buffer = VK_NULL_HANDLE; // the buffer we will record into

    // descriptors
    // gather all uniform variables in different frames into one buffer (fixed size)
    VkDescriptorSet _uniform_data_descriptor_set = VK_NULL_HANDLE;
    // one buffer per frame (unfixed size)
    VkDescriptorSet _object_descriptor_set = VK_NULL_HANDLE;
    AllocatedBuffer _object_buffer{};
};

