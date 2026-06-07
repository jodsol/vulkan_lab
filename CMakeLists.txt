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
    if(WIN32)
        include(FetchContent)

        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(glfw)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GLFW3 REQUIRED IMPORTED_TARGET glfw3)
    endif()
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
    src/experiments/core/ComputeSupport.cpp
    src/experiments/compute_latency/ComputeLatencyExperiment.cpp
    src/experiments/compute_probe/ComputeProbeExperiment.cpp
    src/experiments/core/ExperimentRunner.cpp
    src/experiments/register_pressure/RegisterPressureExperiment.cpp
    src/experiments/swapchain/SwapchainExperiment.cpp
    src/experiments/thread_divergence/ThreadDivergenceExperiment.cpp
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

find_program(GLSLC_EXECUTABLE
    NAMES glslc
    HINTS ENV VULKAN_SDK
    PATH_SUFFIXES Bin bin
)

if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc was not found. Install the Vulkan SDK or make glslc available in PATH.")
endif()

set(SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")
file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

set(COMPUTE_SHADER_SOURCES
    shaders/compute_latency/high_latency.comp
    shaders/compute_latency/high_occupancy.comp
    shaders/register_pressure/low.comp
    shaders/register_pressure/medium.comp
    shaders/register_pressure/high.comp
    shaders/thread_divergence/branch_patterns.comp
)

set(COMPUTE_SHADER_OUTPUTS "")
foreach(SHADER_SOURCE IN ITEMS
    shaders/compute_latency/high_latency.comp
    shaders/compute_latency/high_occupancy.comp
)
    get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME)
    set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv")
    list(APPEND COMPUTE_SHADER_OUTPUTS "${SHADER_OUTPUT}")

    add_custom_command(
        OUTPUT "${SHADER_OUTPUT}"
        COMMAND "${GLSLC_EXECUTABLE}" -fshader-stage=compute "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}" -o "${SHADER_OUTPUT}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}"
        COMMENT "Compiling shader ${SHADER_NAME}"
        VERBATIM
    )
endforeach()

set(REGISTER_PRESSURE_LEVELS low medium high)
set(REGISTER_PRESSURE_LOCAL_SIZES 32 64 128 256)

foreach(PRESSURE_LEVEL IN LISTS REGISTER_PRESSURE_LEVELS)
    foreach(LOCAL_SIZE IN LISTS REGISTER_PRESSURE_LOCAL_SIZES)
        set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/register_pressure/${PRESSURE_LEVEL}.comp")
        set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/register_pressure_${PRESSURE_LEVEL}_${LOCAL_SIZE}.comp.spv")
        list(APPEND COMPUTE_SHADER_OUTPUTS "${SHADER_OUTPUT}")

        add_custom_command(
            OUTPUT "${SHADER_OUTPUT}"
            COMMAND "${GLSLC_EXECUTABLE}"
                -fshader-stage=compute
                -DLOCAL_SIZE_X=${LOCAL_SIZE}
                "${SHADER_SOURCE}"
                -o "${SHADER_OUTPUT}"
            DEPENDS "${SHADER_SOURCE}"
            COMMENT "Compiling register pressure shader ${PRESSURE_LEVEL} local_size=${LOCAL_SIZE}"
            VERBATIM
        )
    endforeach()
endforeach()

set(THREAD_DIVERGENCE_CASES
    no-branch:0
    uniform:1
    coherent:2
    divergent:3
)
set(THREAD_DIVERGENCE_LOCAL_SIZES 32 64 128 256)

foreach(CASE_INFO IN LISTS THREAD_DIVERGENCE_CASES)
    string(REPLACE ":" ";" CASE_PARTS "${CASE_INFO}")
    list(GET CASE_PARTS 0 CASE_NAME)
    list(GET CASE_PARTS 1 BRANCH_PATTERN)

    foreach(LOCAL_SIZE IN LISTS THREAD_DIVERGENCE_LOCAL_SIZES)
        set(SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/thread_divergence/branch_patterns.comp")
        set(SHADER_OUTPUT "${SHADER_OUTPUT_DIR}/thread_divergence_${CASE_NAME}_${LOCAL_SIZE}.comp.spv")
        list(APPEND COMPUTE_SHADER_OUTPUTS "${SHADER_OUTPUT}")

        add_custom_command(
            OUTPUT "${SHADER_OUTPUT}"
            COMMAND "${GLSLC_EXECUTABLE}"
                -fshader-stage=compute
                --target-env=vulkan1.1
                -DLOCAL_SIZE_X=${LOCAL_SIZE}
                -DBRANCH_PATTERN=${BRANCH_PATTERN}
                "${SHADER_SOURCE}"
                -o "${SHADER_OUTPUT}"
            DEPENDS "${SHADER_SOURCE}"
            COMMENT "Compiling thread divergence shader ${CASE_NAME} local_size=${LOCAL_SIZE}"
            VERBATIM
        )
    endforeach()
endforeach()

add_custom_target(compile_shaders DEPENDS ${COMPUTE_SHADER_OUTPUTS})
add_dependencies(VulkanEngine compile_shaders)

target_compile_definitions(VulkanEngine
    PRIVATE
        VULKAN_EXPERIMENT_SHADER_DIR="${SHADER_OUTPUT_DIR}"
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
