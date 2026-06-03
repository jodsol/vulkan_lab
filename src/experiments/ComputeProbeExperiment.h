#pragma once

#include "experiments/Experiment.h"

class ComputeProbeExperiment final : public Experiment {
public:
    std::string name() const override;
    ExperimentRequirements requirements() const override;
    void setup(VulkanContext& context) override;
    void run(VulkanContext& context) override;
    void teardown(VulkanContext& context) override;
};
