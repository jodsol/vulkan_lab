#pragma once

#include "vulkan/PhysicalDevice.h"

#include <vector>
#include <vulkan/vulkan.h>

class LogicalDevice {
public:
    LogicalDevice(
        const PhysicalDevice& physicalDevice,
        const std::vector<const char*>& requiredExtensions);
    ~LogicalDevice();

    LogicalDevice(const LogicalDevice&) = delete;
    LogicalDevice& operator=(const LogicalDevice&) = delete;

    VkDevice handle() const;
    VkQueue graphicsQueue() const;
    VkQueue presentQueue() const;
    VkQueue computeQueue() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkQueue computeQueue_ = VK_NULL_HANDLE;
};
