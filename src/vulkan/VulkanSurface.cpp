#include "vulkan/VulkanSurface.h"

VulkanSurface::VulkanSurface(VkInstance instance, const VulkanWindow& window)
    : instance_(instance),
      surface_(window.createSurface(instance)) {}

VulkanSurface::~VulkanSurface() {
    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }
}

VkSurfaceKHR VulkanSurface::handle() const {
    return surface_;
}
