#pragma once

#include "vulkan/LogicalDevice.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/VulkanWindow.h"

#include <vector>
#include <vulkan/vulkan.h>

class Swapchain {
public:
    Swapchain(
        const VulkanWindow& window,
        const PhysicalDevice& physicalDevice,
        const LogicalDevice& logicalDevice,
        VkSurfaceKHR surface);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    VkFormat imageFormat() const;
    VkExtent2D extent() const;
    const std::vector<VkImageView>& imageViews() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat imageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;

    void createImageViews();
};
