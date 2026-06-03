#include "experiments/ExperimentRunner.h"

#include <stdexcept>
#include <utility>

ExperimentRunner::ExperimentRunner(std::vector<std::unique_ptr<Experiment>> experiments)
    : experiments_(std::move(experiments)) {}

void ExperimentRunner::run(const std::string& experimentName) {
    for (const auto& experiment : experiments_) {
        if (experiment->name() == experimentName) {
            experiment->run();
            return;
        }
    }

    throw std::runtime_error("Unknown experiment: " + experimentName);
}
