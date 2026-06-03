cmake_minimum_required(VERSION 3.20)

project(CrossPlatformVulkanEngine
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG QUIET)

if(NOT glfw3_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)
endif()

set(GLFW_TARGET "")
if(TARGET glfw)
    set(GLFW_TARGET glfw)
elseif(TARGET glfw3)
    set(GLFW_TARGET glfw3)
elseif(TARGET glfw::glfw)
    set(GLFW_TARGET glfw::glfw)
elseif(TARGET PkgConfig::GLFW3)
    set(GLFW_TARGET PkgConfig::GLFW3)
else()
    message(FATAL_ERROR "GLFW was found, but no usable CMake target was exported.")
endif()

add_executable(VulkanEngine
    main.cpp
    src/experiments/ComputeProbeExperiment.cpp
    src/experiments/ExperimentRunner.cpp
    src/experiments/SwapchainExperiment.cpp
    src/vulkan/CommandPool.cpp
    src/vulkan/DebugMessenger.cpp
    src/vulkan/GraphicsPipeline.cpp
    src/vulkan/LogicalDevice.cpp
    src/vulkan/PhysicalDevice.cpp
    src/vulkan/QueueFamilies.cpp
    src/vulkan/RenderPass.cpp
    src/vulkan/Swapchain.cpp
    src/vulkan/SwapchainSupport.cpp
    src/vulkan/VulkanContext.cpp
    src/vulkan/VulkanInstance.cpp
    src/vulkan/VulkanSurface.cpp
    src/vulkan/VulkanWindow.cpp
)

target_include_directories(VulkanEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(VulkanEngine
    PRIVATE
        Vulkan::Vulkan
        ${GLFW_TARGET}
)

target_compile_features(VulkanEngine PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(VulkanEngine PRIVATE /W4)
else()
    target_compile_options(VulkanEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
