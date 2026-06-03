#pragma once

#include <vulkan/vulkan.h>

class CommandPool {
public:
    CommandPool(VkDevice device, uint32_t queueFamilyIndex);
    ~CommandPool();

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    VkCommandPool handle() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
};
