#pragma once

#include "experiments/Experiment.h"

#include <memory>
#include <string>
#include <vector>

class ExperimentRunner {
public:
    explicit ExperimentRunner(std::vector<std::unique_ptr<Experiment>> experiments);

    void run(const std::string& experimentName);
    std::vector<std::string> experimentNames() const;

private:
    std::vector<std::unique_ptr<Experiment>> experiments_;
};
