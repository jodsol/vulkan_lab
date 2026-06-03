#pragma once

#include <string>

class VulkanContext;

struct ExperimentRequirements {
    bool requiresWindow = false;
    bool requiresSwapchain = false;
    int windowWidth = 1280;
    int windowHeight = 720;
    const char* windowTitle = "Vulkan Experiment";
};

class Experiment {
public:
    virtual ~Experiment() = default;

    virtual std::string name() const = 0;
    virtual ExperimentRequirements requirements() const = 0;
    virtual void setup(VulkanContext& context) = 0;
    virtual void run(VulkanContext& context) = 0;
    virtual void teardown(VulkanContext& context) = 0;
};
