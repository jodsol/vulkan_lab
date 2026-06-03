#include "experiments/ExperimentRunner.h"
#include "experiments/SwapchainExperiment.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

int main() {
    try {
        std::vector<std::unique_ptr<Experiment>> experiments;
        experiments.push_back(std::make_unique<SwapchainExperiment>());

        ExperimentRunner runner(std::move(experiments));
        runner.run("swapchain");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
