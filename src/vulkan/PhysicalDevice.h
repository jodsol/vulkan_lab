#pragma once

#include "vulkan/QueueFamilies.h"
#include "vulkan/SwapchainSupport.h"

#include <vector>
#include <vulkan/vulkan.h>

class PhysicalDevice {
public:
    PhysicalDevice(
        VkInstance instance,
        VkSurfaceKHR surface,
        const std::vector<const char*>& requiredExtensions,
        bool requiresPresent);

    VkPhysicalDevice handle() const;
    const QueueFamilyIndices& queueFamilies() const;
    SwapchainSupportDetails querySwapchainSupport(VkSurfaceKHR surface) const;

private:
    VkPhysicalDevice device_ = VK_NULL_HANDLE;
    QueueFamilyIndices queueFamilies_;
};
