#pragma once

#include "experiments/Experiment.h"
#include "vulkan/CommandPool.h"
#include "vulkan/LogicalDevice.h"
#include "vulkan/PhysicalDevice.h"
#include "vulkan/RenderPass.h"
#include "vulkan/Swapchain.h"
#include "vulkan/VulkanInstance.h"
#include "vulkan/VulkanSurface.h"
#include "vulkan/VulkanWindow.h"

#include <memory>
#include <vector>

class VulkanContext {
public:
    explicit VulkanContext(const ExperimentRequirements& requirements);

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    void createGraphicsResources();
    void waitIdle() const;

    const ExperimentRequirements& requirements() const;
    VulkanInstance& instance();
    const VulkanInstance& instance() const;
    PhysicalDevice& physicalDevice();
    const PhysicalDevice& physicalDevice() const;
    LogicalDevice& logicalDevice();
    const LogicalDevice& logicalDevice() const;
    CommandPool& graphicsCommandPool();
    const CommandPool& graphicsCommandPool() const;
    CommandPool& computeCommandPool();
    const CommandPool& computeCommandPool() const;
    VulkanWindow& window();
    const VulkanWindow& window() const;
    VulkanSurface& surface();
    const VulkanSurface& surface() const;
    Swapchain& swapchain();
    const Swapchain& swapchain() const;
    RenderPass& renderPass();
    const RenderPass& renderPass() const;
    bool hasWindow() const;
    bool hasSwapchain() const;

private:
    ExperimentRequirements requirements_;
    std::vector<const char*> deviceExtensions_;
    std::unique_ptr<VulkanWindow> window_;
    VulkanInstance instance_;
    std::unique_ptr<VulkanSurface> surface_;
    PhysicalDevice physicalDevice_;
    LogicalDevice logicalDevice_;
    CommandPool graphicsCommandPool_;
    CommandPool computeCommandPool_;
    std::unique_ptr<Swapchain> swapchain_;
    std::unique_ptr<RenderPass> renderPass_;
};
