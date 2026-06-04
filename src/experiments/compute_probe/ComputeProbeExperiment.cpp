#include "experiments/compute_probe/ComputeProbeExperiment.h"

#include "vulkan/VulkanContext.h"

#include <iostream>

std::string ComputeProbeExperiment::name() const {
    return "compute-probe";
}

ExperimentRequirements ComputeProbeExperiment::requirements() const {
    ExperimentRequirements requirements;
    requirements.windowTitle = "Vulkan Experiment: Compute Probe";
    return requirements;
}

void ComputeProbeExperiment::setup(VulkanContext& context) {
    (void)context;
}

void ComputeProbeExperiment::run(VulkanContext& context) {
    const QueueFamilyIndices& queues = context.physicalDevice().queueFamilies();
    std::cout
        << "Compute experiment context is ready.\n"
        << "graphics queue family: " << queues.graphicsFamily.value() << '\n'
        << "compute queue family: " << queues.computeFamily.value() << '\n';
}

void ComputeProbeExperiment::teardown(VulkanContext& context) {
    context.waitIdle();
}
