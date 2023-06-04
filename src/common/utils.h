#pragma once

#include <functional>
#include <queue>
#include <deque>
#include <iostream>

#include <glm/glm.hpp>

#include "types.h"

// check the vulkan result
#define VK_CHECK(x)                                                         \
do {                                                                        \
    VkResult err = x;                                                       \
    if (err) {                                                              \
        std::cerr << "[Error] Detected Vulkan Error: " << err << std::endl; \
        abort();                                                            \
    }                                                                       \
} while (0)                                                                 \

template <typename T>
class FixSizeQueue {
public:
    FixSizeQueue(size_t max_size) : _max_size(max_size) {}

    void push(const T& value) {
        _queue.push(value);
        constrain_size();
    }

    template <typename... Args> void emplace(Args &&...args) {
        _queue.emplace(std::forward<Args>(args)...);
        constrain_size();
    }

    const T& front() const {
        return _queue.front();
    }

    const T& back() const {
        return _queue.back();
    }

    bool empty() const {
        return _queue.empty();
    }

    size_t size() const {
        return _queue.size();
    }

private:
    void constrain_size() {
        while (_queue.size() > _max_size) {
            _queue.pop();
        }
    }

    size_t _max_size;
    std::queue<T> _queue{};
};

class DeletionQueue {
public:
    std::deque<std::function<void()>> deletors{};

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse
        for (auto it = deletors.rbegin(); it < deletors.rend(); ++it) {
            (*it)();
        }
        deletors.clear();
    }
};

namespace vkutils {
    uint32_t padding(uint32_t original_size, uint32_t alignment);

    glm::vec3 polar_to_cartesian(float yaw, float pitch);
}

