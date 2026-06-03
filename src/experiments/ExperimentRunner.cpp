#include "experiments/ExperimentRunner.h"

#include "vulkan/VulkanContext.h"

#include <stdexcept>
#include <utility>

ExperimentRunner::ExperimentRunner(std::vector<std::unique_ptr<Experiment>> experiments)
    : experiments_(std::move(experiments)) {}

void ExperimentRunner::run(const std::string& experimentName) {
    for (const auto& experiment : experiments_) {
        if (experiment->name() == experimentName) {
            VulkanContext context(experiment->requirements());
            bool isSetupComplete = false;

            try {
                experiment->setup(context);
                isSetupComplete = true;
                experiment->run(context);
            } catch (...) {
                if (isSetupComplete) {
                    experiment->teardown(context);
                }

                throw;
            }

            experiment->teardown(context);
            return;
        }
    }

    throw std::runtime_error("Unknown experiment: " + experimentName);
}

std::vector<std::string> ExperimentRunner::experimentNames() const {
    std::vector<std::string> names;
    names.reserve(experiments_.size());

    for (const auto& experiment : experiments_) {
        names.push_back(experiment->name());
    }

    return names;
}
