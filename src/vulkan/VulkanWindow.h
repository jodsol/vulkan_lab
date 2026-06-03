#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanWindow {
public:
    VulkanWindow(int width, int height, const char* title);
    ~VulkanWindow();

    VulkanWindow(const VulkanWindow&) = delete;
    VulkanWindow& operator=(const VulkanWindow&) = delete;

    bool shouldClose() const;
    VkExtent2D framebufferExtent() const;
    VkSurfaceKHR createSurface(VkInstance instance) const;

private:
    GLFWwindow* window_ = nullptr;
};
