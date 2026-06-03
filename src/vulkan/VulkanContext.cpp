#include "vulkan/VulkanContext.h"

#include <GLFW/glfw3.h>

#include <stdexcept>

namespace {
std::vector<const char*> requiredInstanceExtensions(bool requiresWindow) {
    if (!requiresWindow) {
        return {};
    }

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (glfwExtensions == nullptr || glfwExtensionCount == 0) {
        throw std::runtime_error("GLFW could not provide Vulkan instance extensions.");
    }

    return {glfwExtensions, glfwExtensions + glfwExtensionCount};
}

std::vector<const char*> requiredDeviceExtensions(const ExperimentRequirements& requirements) {
    if (requirements.requiresSwapchain) {
        return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    }

    return {};
}
}  // namespace

VulkanContext::VulkanContext(const ExperimentRequirements& requirements)
    : requirements_(requirements),
      deviceExtensions_(requiredDeviceExtensions(requirements)),
      window_(requirements.requiresWindow
                  ? std::make_unique<VulkanWindow>(
                        requirements.windowWidth,
                        requirements.windowHeight,
                        requirements.windowTitle)
                  : nullptr),
      instance_("Vulkan Experiments", requiredInstanceExtensions(requirements.requiresWindow)),
      surface_(requirements.requiresWindow
                   ? std::make_unique<VulkanSurface>(instance_.handle(), *window_)
                   : nullptr),
      physicalDevice_(
          instance_.handle(),
          surface_ != nullptr ? surface_->handle() : VK_NULL_HANDLE,
          deviceExtensions_,
          requirements.requiresSwapchain),
      logicalDevice_(physicalDevice_, deviceExtensions_),
      graphicsCommandPool_(logicalDevice_.handle(), physicalDevice_.queueFamilies().graphicsFamily.value()),
      computeCommandPool_(logicalDevice_.handle(), physicalDevice_.queueFamilies().computeFamily.value()) {}

void VulkanContext::createGraphicsResources() {
    if (!requirements_.requiresSwapchain) {
        throw std::runtime_error("Graphics resources requested without swapchain support.");
    }

    if (swapchain_ == nullptr) {
        swapchain_ = std::make_unique<Swapchain>(*window_, physicalDevice_, logicalDevice_, surface_->handle());
    }

    if (renderPass_ == nullptr) {
        renderPass_ = std::make_unique<RenderPass>(logicalDevice_.handle(), swapchain_->imageFormat());
    }
}

void VulkanContext::waitIdle() const {
    vkDeviceWaitIdle(logicalDevice_.handle());
}

const ExperimentRequirements& VulkanContext::requirements() const {
    return requirements_;
}

VulkanInstance& VulkanContext::instance() {
    return instance_;
}

const VulkanInstance& VulkanContext::instance() const {
    return instance_;
}

PhysicalDevice& VulkanContext::physicalDevice() {
    return physicalDevice_;
}

const PhysicalDevice& VulkanContext::physicalDevice() const {
    return physicalDevice_;
}

LogicalDevice& VulkanContext::logicalDevice() {
    return logicalDevice_;
}

const LogicalDevice& VulkanContext::logicalDevice() const {
    return logicalDevice_;
}

CommandPool& VulkanContext::graphicsCommandPool() {
    return graphicsCommandPool_;
}

const CommandPool& VulkanContext::graphicsCommandPool() const {
    return graphicsCommandPool_;
}

CommandPool& VulkanContext::computeCommandPool() {
    return computeCommandPool_;
}

const CommandPool& VulkanContext::computeCommandPool() const {
    return computeCommandPool_;
}

VulkanWindow& VulkanContext::window() {
    if (window_ == nullptr) {
        throw std::runtime_error("This experiment does not use a window.");
    }

    return *window_;
}

const VulkanWindow& VulkanContext::window() const {
    if (window_ == nullptr) {
        throw std::runtime_error("This experiment does not use a window.");
    }

    return *window_;
}

VulkanSurface& VulkanContext::surface() {
    if (surface_ == nullptr) {
        throw std::runtime_error("This experiment does not use a surface.");
    }

    return *surface_;
}

const VulkanSurface& VulkanContext::surface() const {
    if (surface_ == nullptr) {
        throw std::runtime_error("This experiment does not use a surface.");
    }

    return *surface_;
}

Swapchain& VulkanContext::swapchain() {
    if (swapchain_ == nullptr) {
        throw std::runtime_error("Swapchain resources have not been created.");
    }

    return *swapchain_;
}

const Swapchain& VulkanContext::swapchain() const {
    if (swapchain_ == nullptr) {
        throw std::runtime_error("Swapchain resources have not been created.");
    }

    return *swapchain_;
}

RenderPass& VulkanContext::renderPass() {
    if (renderPass_ == nullptr) {
        throw std::runtime_error("Render pass resources have not been created.");
    }

    return *renderPass_;
}

const RenderPass& VulkanContext::renderPass() const {
    if (renderPass_ == nullptr) {
        throw std::runtime_error("Render pass resources have not been created.");
    }

    return *renderPass_;
}

bool VulkanContext::hasWindow() const {
    return window_ != nullptr;
}

bool VulkanContext::hasSwapchain() const {
    return swapchain_ != nullptr;
}
