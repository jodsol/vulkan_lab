#include "experiments/SwapchainExperiment.h"

#include "vulkan/LogicalDevice.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/RenderPass.h"
#include "vulkan/Swapchain.h"
#include "vulkan/VulkanSurface.h"
#include "vulkan/VulkanInstance.h"
#include "vulkan/VulkanWindow.h"

#include <GLFW/glfw3.h>

std::string SwapchainExperiment::name() const {
    return "swapchain";
}

void SwapchainExperiment::run() {
    VulkanWindow window(1280, 720, "Vulkan Experiment: Swapchain");
    VulkanInstance instance("Vulkan Experiments");
    VulkanSurface surface(instance.handle(), window);

    PhysicalDevice physicalDevice(instance.handle(), surface.handle());
    LogicalDevice logicalDevice(physicalDevice);
    Swapchain swapchain(window, physicalDevice, logicalDevice, surface.handle());
    RenderPass renderPass(logicalDevice.handle(), swapchain.imageFormat());

    while (!window.shouldClose()) {
        glfwPollEvents();
    }

    vkDeviceWaitIdle(logicalDevice.handle());
}
