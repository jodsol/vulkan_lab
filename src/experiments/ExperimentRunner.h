#pragma once

#include "experiments/Experiment.h"

#include <memory>
#include <string>
#include <vector>

class ExperimentRunner {
public:
    explicit ExperimentRunner(std::vector<std::unique_ptr<Experiment>> experiments);

    void run(const std::string& experimentName);

private:
    std::vector<std::unique_ptr<Experiment>> experiments_;
};
