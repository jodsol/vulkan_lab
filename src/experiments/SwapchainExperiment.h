#pragma once

#include "experiments/Experiment.h"

class SwapchainExperiment final : public Experiment {
public:
    std::string name() const override;
    void run() override;
};
