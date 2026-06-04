#include "experiments/swapchain/SwapchainExperiment.h"

#include "vulkan/VulkanContext.h"

#include <GLFW/glfw3.h>

std::string SwapchainExperiment::name() const {
    return "swapchain";
}

ExperimentRequirements SwapchainExperiment::requirements() const {
    ExperimentRequirements requirements;
    requirements.requiresWindow = true;
    requirements.requiresSwapchain = true;
    requirements.windowTitle = "Vulkan Experiment: Swapchain";
    return requirements;
}

void SwapchainExperiment::setup(VulkanContext& context) {
    context.createGraphicsResources();
}

void SwapchainExperiment::run(VulkanContext& context) {
    while (!context.window().shouldClose()) {
        glfwPollEvents();
    }
}

void SwapchainExperiment::teardown(VulkanContext& context) {
    context.waitIdle();
}
