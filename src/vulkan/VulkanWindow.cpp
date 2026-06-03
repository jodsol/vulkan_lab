#include "vulkan/VulkanWindow.h"

#include <stdexcept>

VulkanWindow::VulkanWindow(int width, int height, const char* title) {
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("Failed to initialize GLFW.");
    }

    if (glfwVulkanSupported() != GLFW_TRUE) {
        throw std::runtime_error("GLFW did not find Vulkan loader support.");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window_ == nullptr) {
        throw std::runtime_error("Failed to create GLFW window.");
    }
}

VulkanWindow::~VulkanWindow() {
    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
    }

    glfwTerminate();
}

bool VulkanWindow::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

VkExtent2D VulkanWindow::framebufferExtent() const {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);

    return {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };
}

VkSurfaceKHR VulkanWindow::createSurface(VkInstance instance) const {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (glfwCreateWindowSurface(instance, window_, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan window surface.");
    }

    return surface;
}
