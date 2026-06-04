#include "experiments/register_pressure/RegisterPressureExperiment.h"

#include "experiments/core/ComputeSupport.h"
#include "vulkan/VulkanContext.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr uint32_t kElementCount = 262144;
constexpr uint32_t kIterationCount = 1024;

struct PushConstants {
    uint32_t elementCount;
    uint32_t iterationCount;
};

struct Variant {
    const char* pressureName;
    uint32_t localSize;
    std::string shaderFilename() const {
        return std::string("register_pressure_") + pressureName + "_" + std::to_string(localSize) + ".comp.spv";
    }

    std::string label() const {
        return std::string(pressureName) + "-ls" + std::to_string(localSize);
    }
};

struct Result {
    Variant variant;
    double gpuMilliseconds;
};

std::vector<float> makeInputData() {
    std::vector<float> values(kElementCount);
    for (uint32_t i = 0; i < kElementCount; ++i) {
        values[i] = static_cast<float>((i % 251U) * 0.125F + 1.0F);
    }
    return values;
}

std::vector<Variant> variants() {
    const std::array<const char*, 3> pressures = {"low", "medium", "high"};
    const std::array<uint32_t, 4> localSizes = {32, 64, 128, 256};

    std::vector<Variant> result;
    for (const char* pressure : pressures) {
        for (uint32_t localSize : localSizes) {
            result.push_back({pressure, localSize});
        }
    }

    return result;
}

std::array<float, 4> labelColor(const std::string& pressure) {
    if (pressure == "low") {
        return {0.24F, 0.70F, 0.29F, 1.0F};
    }

    if (pressure == "medium") {
        return {0.98F, 0.76F, 0.14F, 1.0F};
    }

    return {0.88F, 0.27F, 0.27F, 1.0F};
}
}  // namespace

std::string RegisterPressureExperiment::name() const {
    return "register-pressure";
}

ExperimentRequirements RegisterPressureExperiment::requirements() const {
    return {};
}

void RegisterPressureExperiment::setup(VulkanContext& context) {
    (void)context;
}

void RegisterPressureExperiment::run(VulkanContext& context) {
    const auto testVariants = variants();
    const VkDevice device = context.logicalDevice().handle();
    const VkPhysicalDevice physicalDevice = context.physicalDevice().handle();
    const VkQueue computeQueue = context.logicalDevice().computeQueue();

    const BufferResource inputBuffer(
        physicalDevice,
        device,
        sizeof(float) * kElementCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    const BufferResource outputBuffer(
        physicalDevice,
        device,
        sizeof(float) * kElementCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    const auto inputData = makeInputData();
    inputBuffer.write(inputData.data(), inputData.size() * sizeof(float));

    const VkDescriptorSetLayout descriptorSetLayout =
        createDescriptorSetLayout(device, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    const VkDescriptorPool descriptorPool =
        createDescriptorPool(device, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2, 1);
    const VkPipelineLayout pipelineLayout =
        createPipelineLayout(device, descriptorSetLayout, sizeof(PushConstants));
    const VkDescriptorSet descriptorSet = allocateDescriptorSet(device, descriptorPool, descriptorSetLayout);
    updateStorageBufferDescriptorSet(device, descriptorSet, {&inputBuffer, &outputBuffer});

    const std::string shaderRoot = VULKAN_EXPERIMENT_SHADER_DIR;
    std::vector<VkPipeline> pipelines;
    pipelines.reserve(testVariants.size());
    for (const Variant& variant : testVariants) {
        pipelines.push_back(createComputePipeline(device, pipelineLayout, shaderRoot + "/" + variant.shaderFilename()));
    }

    const VkCommandBuffer commandBuffer =
        allocateCommandBuffer(device, context.computeCommandPool().handle());
    const VkFence fence = createFence(device);
    const VkQueryPool queryPool = createTimestampQueryPool(device, static_cast<uint32_t>(testVariants.size() * 2));
    const auto beginLabel = getBeginDebugLabel(device);
    const auto endLabel = getEndDebugLabel(device);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin register pressure command buffer.");
    }

    vkCmdResetQueryPool(commandBuffer, queryPool, 0, static_cast<uint32_t>(testVariants.size() * 2));

    const PushConstants pushConstants{kElementCount, kIterationCount};

    for (size_t i = 0; i < testVariants.size(); ++i) {
        const Variant& variant = testVariants[i];
        const uint32_t queryBase = static_cast<uint32_t>(i * 2);
        const std::string label = variant.label();

        beginDebugLabel(beginLabel, commandBuffer, label.c_str(), labelColor(variant.pressureName));
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, queryBase);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelines[i]);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout,
            0,
            1,
            &descriptorSet,
            0,
            nullptr);
        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(PushConstants),
            &pushConstants);
        vkCmdDispatch(commandBuffer, kElementCount / variant.localSize, 1, 1);
        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, queryBase + 1);
        endDebugLabel(endLabel, commandBuffer);

        if (i + 1 < testVariants.size()) {
            VkMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                1,
                &barrier,
                0,
                nullptr,
                0,
                nullptr);
        }
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record register pressure command buffer.");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(computeQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit register pressure command buffer.");
    }

    if (vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        throw std::runtime_error("Failed to wait for register pressure experiment completion.");
    }

    std::vector<uint64_t> timestamps(testVariants.size() * 2);
    if (vkGetQueryPoolResults(
            device,
            queryPool,
            0,
            static_cast<uint32_t>(timestamps.size()),
            sizeof(uint64_t) * timestamps.size(),
            timestamps.data(),
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) != VK_SUCCESS) {
        throw std::runtime_error("Failed to read register pressure timestamps.");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    std::vector<Result> results;
    results.reserve(testVariants.size());
    for (size_t i = 0; i < testVariants.size(); ++i) {
        const double nanoseconds =
            timestampDeltaNanoseconds(timestamps[i * 2], timestamps[i * 2 + 1], properties);
        results.push_back({testVariants[i], nanoseconds / 1000000.0});
    }

    const auto outputSamples = outputBuffer.readFloat(4);

    std::cout
        << "RTR Chapter 3 Validation: Register Pressure vs GPU Time\n"
        << "Element count: " << kElementCount << '\n'
        << "Iteration count: " << kIterationCount << '\n'
        << '\n'
        << "| Pressure | Local Size | GPU Time (ms) |\n"
        << "| --- | ---: | ---: |\n";

    std::cout << std::fixed << std::setprecision(4);
    for (const Result& result : results) {
        std::cout
            << "| " << result.variant.pressureName
            << " | " << result.variant.localSize
            << " | " << result.gpuMilliseconds
            << " |\n";
    }

    std::cout
        << '\n'
        << "Output sample[0..3]: "
        << outputSamples[0] << ", "
        << outputSamples[1] << ", "
        << outputSamples[2] << ", "
        << outputSamples[3] << '\n'
        << "For register count and occupancy, capture these shader variants with Nsight Compute.\n";

    vkDestroyQueryPool(device, queryPool, nullptr);
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, context.computeCommandPool().handle(), 1, &commandBuffer);

    for (VkPipeline pipeline : pipelines) {
        vkDestroyPipeline(device, pipeline, nullptr);
    }

    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void RegisterPressureExperiment::teardown(VulkanContext& context) {
    context.waitIdle();
}
