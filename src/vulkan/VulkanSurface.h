#pragma once

#include "vulkan/VulkanWindow.h"

#include <vulkan/vulkan.h>

class VulkanSurface {
public:
    VulkanSurface(VkInstance instance, const VulkanWindow& window);
    ~VulkanSurface();

    VulkanSurface(const VulkanSurface&) = delete;
    VulkanSurface& operator=(const VulkanSurface&) = delete;

    VkSurfaceKHR handle() const;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
};
