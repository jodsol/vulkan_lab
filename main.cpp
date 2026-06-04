#include "experiments/core/ExperimentRunner.h"
#include "experiments/compute_probe/ComputeProbeExperiment.h"
#include "experiments/compute_latency/ComputeLatencyExperiment.h"
#include "experiments/register_pressure/RegisterPressureExperiment.h"
#include "experiments/swapchain/SwapchainExperiment.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char** argv) {
    try {
        std::vector<std::unique_ptr<Experiment>> experiments;
        experiments.push_back(std::make_unique<ComputeLatencyExperiment>());
        experiments.push_back(std::make_unique<ComputeProbeExperiment>());
        experiments.push_back(std::make_unique<RegisterPressureExperiment>());
        experiments.push_back(std::make_unique<SwapchainExperiment>());

        ExperimentRunner runner(std::move(experiments));
        if (argc > 1 && std::string(argv[1]) == "--list") {
            for (const auto& name : runner.experimentNames()) {
                std::cout << name << '\n';
            }

            return EXIT_SUCCESS;
        }

        const std::string experimentName = argc > 1 ? argv[1] : "swapchain";
        runner.run(experimentName);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
