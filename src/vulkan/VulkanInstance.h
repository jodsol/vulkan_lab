#pragma once

#include "vulkan/DebugMessenger.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanInstance {
public:
    VulkanInstance(const char* applicationName, std::vector<const char*> requiredExtensions);
    ~VulkanInstance();

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    VkInstance handle() const;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    std::unique_ptr<DebugMessenger> debugMessenger_;
};
