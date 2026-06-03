#pragma once

#include "vulkan/PhysicalDevice.h"

#include <vulkan/vulkan.h>

class LogicalDevice {
public:
    explicit LogicalDevice(const PhysicalDevice& physicalDevice);
    ~LogicalDevice();

    LogicalDevice(const LogicalDevice&) = delete;
    LogicalDevice& operator=(const LogicalDevice&) = delete;

    VkDevice handle() const;
    VkQueue graphicsQueue() const;
    VkQueue presentQueue() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
};
