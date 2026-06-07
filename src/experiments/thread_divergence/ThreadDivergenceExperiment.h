#pragma once

#include "experiments/core/Experiment.h"

class ThreadDivergenceExperiment final : public Experiment {
public:
    std::string name() const override;
    ExperimentRequirements requirements() const override;
    void setup(VulkanContext& context) override;
    void run(VulkanContext& context) override;
    void teardown(VulkanContext& context) override;
};
