#pragma once

#include <optional>
#include <vulkan/vulkan.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;

    bool hasGraphics() const {
        return graphicsFamily.has_value();
    }

    bool hasPresent() const {
        return presentFamily.has_value();
    }

    bool hasCompute() const {
        return computeFamily.has_value();
    }

    bool isComplete(bool requiresPresent) const {
        if (!hasGraphics() || !hasCompute()) {
            return false;
        }

        return !requiresPresent || hasPresent();
    }
};

QueueFamilyIndices findQueueFamilies(
    VkPhysicalDevice device,
    VkSurfaceKHR surface = VK_NULL_HANDLE,
    bool requiresPresent = false);
