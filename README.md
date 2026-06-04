# Vulkan Experiments

This repository is a Vulkan experiment playground.

The project is structured to make it easier to add and run many kinds of Vulkan experiments over time, including both graphics and compute workloads.

## Goals

- add experiments without rewriting the application entry flow
- support both windowed graphics experiments and headless compute experiments
- keep shared Vulkan setup in one reusable context
- make it easy to compare different approaches over time

## Current Structure

The project is centered around three ideas:

- `Experiment`
  each experiment declares what it needs and implements `setup`, `run`, and `teardown`
- `ExperimentRunner`
  chooses one experiment and executes it
- `VulkanContext`
  owns the common Vulkan state used by experiments

Important files:

- [main.cpp](C:/git/vulkan/main.cpp:1)
- [Experiment.h](C:/git/vulkan/src/experiments/core/Experiment.h:1)
- [ExperimentRunner.cpp](C:/git/vulkan/src/experiments/core/ExperimentRunner.cpp:1)
- [VulkanContext.h](C:/git/vulkan/src/vulkan/VulkanContext.h:1)

More detailed notes are in [project-overview.md](C:/git/vulkan/docs/project-overview.md:1).

## Current Experiments

### `swapchain`

A windowed experiment that verifies the graphics-side initialization path.

It currently creates:

- a window
- a Vulkan instance
- a surface
- a physical device
- a logical device
- a swapchain
- swapchain image views
- a render pass

Expected result:

- a console window may show validation layer messages in Debug builds
- a normal window titled `Vulkan Experiment: Swapchain` opens
- the window looks mostly blank
- closing the window exits the app normally

The blank window is expected right now because full rendering has not been implemented yet.

### `compute-probe`

A small headless experiment that verifies the compute-friendly path of the framework.

Expected result:

- no window opens
- console output prints queue family information
- the process exits normally

### `compute-latency`

A headless compute experiment designed for RenderDoc capture and basic GPU timestamp comparison.

It records two dispatches in one command buffer:

- `High Latency Dispatch`
- `High Occupancy Dispatch`

Expected result:

- no window opens
- console output prints GPU timing results and output samples
- RenderDoc shows two labeled compute dispatches that can be compared side by side

## Running Experiments

Examples:

```text
VulkanEngine.exe --list
VulkanEngine.exe swapchain
VulkanEngine.exe compute-probe
VulkanEngine.exe compute-latency
```

`--list` prints the available experiment names.

## Build Notes

The project uses CMake.

Relevant files:

- [CMakeLists.txt](C:/git/vulkan/CMakeLists.txt:1)
- [CMakePresets.json](C:/git/vulkan/CMakePresets.json:1)

On a machine where the configured compiler is available, the general flow is:

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
```

Depending on your environment, another preset may be more appropriate.

## Current Status

This repository is not yet a full renderer or benchmark suite.

What it does already provide is a base for adding experiments cleanly.
Good next candidates include:

- clear color rendering
- triangle rendering
- compute latency tests
- draw call benchmarks
