# Project Overview

## What This Project Is

This project is a Vulkan experiment playground.

The goal is not to build one fixed renderer, but to create a codebase where many Vulkan experiments can be added and run with as little friction as possible.

Examples of the kinds of experiments this structure is meant to support:

- compute shader latency comparisons
- draw call count comparisons
- pipeline state change experiments
- descriptor update experiments
- queue usage and synchronization experiments

## Current Design Direction

The project is now organized around an experiment-oriented structure.

Instead of putting Vulkan setup and execution logic directly inside `main.cpp`, the code is split into:

- an experiment interface
- an experiment runner
- a reusable Vulkan context
- Vulkan helper components such as device, queue, swapchain, and render pass management

This makes it easier to add new experiments without rebuilding the whole application structure each time.

## Important Files

- [main.cpp](C:/git/vulkan/main.cpp:1)
  selects and runs an experiment
- [Experiment.h](C:/git/vulkan/src/experiments/Experiment.h:1)
  defines the experiment lifecycle
- [ExperimentRunner.cpp](C:/git/vulkan/src/experiments/ExperimentRunner.cpp:1)
  creates a `VulkanContext` and executes one experiment
- [VulkanContext.h](C:/git/vulkan/src/vulkan/VulkanContext.h:1)
  owns shared Vulkan state used by experiments

## Experiment Lifecycle

Each experiment follows the same lifecycle:

1. `requirements()`
2. `setup(context)`
3. `run(context)`
4. `teardown(context)`

This is useful because different experiments often need different setup costs and execution styles.

For example:

- a graphics experiment may need a window and swapchain
- a compute experiment may run headless without a window
- a benchmark experiment may need repeated runs and timing output

## Current Built-In Experiments

### `swapchain`

This is the current windowed graphics-side experiment.

It requests:

- a GLFW window
- a Vulkan surface
- a swapchain
- a render pass

Its current purpose is to verify that the graphics initialization path works inside the new experiment framework.

### `compute-probe`

This is a small headless compute-oriented experiment.

It does not create a window or swapchain.
Instead, it verifies that the project can create a Vulkan context suitable for compute-style experiments and prints queue family information to the console.

Its purpose is not benchmarking yet.
Its purpose is to prove that the framework can support non-windowed experiments too.

## What Should Happen When Build And Run Succeeds

The expected result depends on which experiment you run.

### When running `swapchain`

You should see:

- a console window with validation layer messages in Debug builds
- a normal application window titled `Vulkan Experiment: Swapchain`

The window itself will currently look mostly blank.
That is expected.

Right now this experiment validates initialization, not full rendering.
So a successful result is:

- the window opens
- the process stays alive until you close the window
- the app exits normally after closing it

You should **not** expect a triangle yet.
You should also **not** expect animated rendering yet.

### When running `compute-probe`

You should see console output similar to:

```text
Compute experiment context is ready.
graphics queue family: 0
compute queue family: 0
```

The exact queue family numbers may differ by GPU and driver.

This experiment should finish without opening a window.

## Why The `swapchain` Window Looks Blank

The project already creates enough Vulkan objects to support future graphics experiments, but it does not yet perform the full rendering loop.

That means these pieces are still missing from the visible rendering path:

- framebuffer creation
- command buffer recording
- image acquire / submit / present flow
- actual draw commands

So a blank window currently means:

`initialization succeeded, rendering has not been implemented yet`

## How To Run

`main.cpp` supports selecting experiments by name.

Examples:

```text
VulkanEngine.exe --list
VulkanEngine.exe swapchain
VulkanEngine.exe compute-probe
```

`--list` should print the available experiment names.

## Practical Goal Of This Structure

This structure is intended to make the next experiments easier to add.

Good next candidates include:

- `clear-color`
- `triangle`
- `compute-latency`
- `drawcall-benchmark`

Each of those can now be added as a separate experiment without rewriting the whole application entry flow.
