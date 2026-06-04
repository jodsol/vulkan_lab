#include "experiments/compute_latency/ComputeLatencyExperiment.h"

#include "vulkan/VulkanContext.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr uint32_t kElementCount = 65536;
constexpr uint32_t kPointerChaseIterations = 256;
constexpr uint32_t kOccupancyIterations = 128;
constexpr uint32_t kHighLatencyLocalSize = 64;
constexpr uint32_t kHighOccupancyLocalSize = 256;

struct PushConstants {
    uint32_t elementCount;
    uint32_t iterationCount;
};

struct BufferResource {
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;

    BufferResource() = default;

    BufferResource(
        VkPhysicalDevice physicalDevice,
        VkDevice logicalDevice,
        VkDeviceSize bufferSize,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties)
        : device(logicalDevice),
          size(bufferSize) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute experiment buffer.");
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        uint32_t memoryTypeIndex = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            const bool typeMatches = (memoryRequirements.memoryTypeBits & (1U << i)) != 0;
            const bool propertiesMatch =
                (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

            if (typeMatches && propertiesMatch) {
                memoryTypeIndex = i;
                break;
            }
        }

        if (memoryTypeIndex == std::numeric_limits<uint32_t>::max()) {
            throw std::runtime_error("Failed to find a suitable buffer memory type.");
        }

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;

        if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate compute experiment buffer memory.");
        }

        if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
            throw std::runtime_error("Failed to bind compute experiment buffer memory.");
        }
    }

    BufferResource(const BufferResource&) = delete;
    BufferResource& operator=(const BufferResource&) = delete;

    BufferResource(BufferResource&& other) noexcept
        : device(other.device),
          buffer(other.buffer),
          memory(other.memory),
          size(other.size) {
        other.device = VK_NULL_HANDLE;
        other.buffer = VK_NULL_HANDLE;
        other.memory = VK_NULL_HANDLE;
        other.size = 0;
    }

    BufferResource& operator=(BufferResource&& other) noexcept {
        if (this != &other) {
            destroy();

            device = other.device;
            buffer = other.buffer;
            memory = other.memory;
            size = other.size;

            other.device = VK_NULL_HANDLE;
            other.buffer = VK_NULL_HANDLE;
            other.memory = VK_NULL_HANDLE;
            other.size = 0;
        }

        return *this;
    }

    ~BufferResource() {
        destroy();
    }

    void write(const void* data, size_t dataSize) const {
        void* mapped = nullptr;
        if (vkMapMemory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map compute experiment buffer.");
        }

        std::memcpy(mapped, data, dataSize);
        vkUnmapMemory(device, memory);
    }

    std::vector<uint32_t> readUint32(size_t count) const {
        void* mapped = nullptr;
        if (vkMapMemory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
            throw std::runtime_error("Failed to map compute output buffer.");
        }

        std::vector<uint32_t> values(count);
        std::memcpy(values.data(), mapped, count * sizeof(uint32_t));
        vkUnmapMemory(device, memory);
        return values;
    }

private:
    void destroy() {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }

        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }

        device = VK_NULL_HANDLE;
        size = 0;
    }
};

std::vector<char> readBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        throw std::runtime_error("Shader file is empty: " + path);
    }

    std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute shader module.");
    }

    return shaderModule;
}

VkPipeline createComputePipeline(
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    const std::string& shaderPath) {
    const auto shaderCode = readBinaryFile(shaderPath);
    const VkShaderModule shaderModule = createShaderModule(device, shaderCode);

    VkPipelineShaderStageCreateInfo shaderStage{};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = shaderModule;
    shaderStage.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStage;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(device, shaderModule, nullptr);
        throw std::runtime_error("Failed to create compute pipeline.");
    }

    vkDestroyShaderModule(device, shaderModule, nullptr);
    return pipeline;
}

VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device) {
    std::array<VkDescriptorSetLayoutBinding, 3> bindings{};
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    createInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout.");
    }

    return layout;
}

VkDescriptorPool createDescriptorPool(VkDevice device) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = 6;

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;
    createInfo.maxSets = 2;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool.");
    }

    return pool;
}

VkPipelineLayout createPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges = &pushConstantRange;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline layout.");
    }

    return pipelineLayout;
}

VkCommandBuffer allocateCommandBuffer(VkDevice device, VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate compute experiment command buffer.");
    }

    return commandBuffer;
}

VkFence createFence(VkDevice device) {
    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(device, &createInfo, nullptr, &fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute experiment fence.");
    }

    return fence;
}

VkQueryPool createTimestampQueryPool(VkDevice device) {
    VkQueryPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = 4;

    VkQueryPool queryPool = VK_NULL_HANDLE;
    if (vkCreateQueryPool(device, &createInfo, nullptr, &queryPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create timestamp query pool.");
    }

    return queryPool;
}

VkDescriptorSet allocateDescriptorSet(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout) {
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout;

    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    if (vkAllocateDescriptorSets(device, &allocateInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set.");
    }

    return descriptorSet;
}

void updateDescriptorSet(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    const BufferResource& indexBuffer,
    const BufferResource& valueBuffer,
    const BufferResource& outputBuffer) {
    VkDescriptorBufferInfo indexInfo{};
    indexInfo.buffer = indexBuffer.buffer;
    indexInfo.offset = 0;
    indexInfo.range = indexBuffer.size;

    VkDescriptorBufferInfo valueInfo{};
    valueInfo.buffer = valueBuffer.buffer;
    valueInfo.offset = 0;
    valueInfo.range = valueBuffer.size;

    VkDescriptorBufferInfo outputInfo{};
    outputInfo.buffer = outputBuffer.buffer;
    outputInfo.offset = 0;
    outputInfo.range = outputBuffer.size;

    std::array<VkWriteDescriptorSet, 3> writes{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &indexInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &valueInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[2].pBufferInfo = &outputInfo;

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

PFN_vkCmdBeginDebugUtilsLabelEXT getBeginDebugLabel(VkDevice device) {
    return reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));
}

PFN_vkCmdEndDebugUtilsLabelEXT getEndDebugLabel(VkDevice device) {
    return reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
        vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));
}

void beginDebugLabel(
    PFN_vkCmdBeginDebugUtilsLabelEXT beginLabel,
    VkCommandBuffer commandBuffer,
    const char* labelName,
    const std::array<float, 4>& color) {
    if (beginLabel == nullptr) {
        return;
    }

    VkDebugUtilsLabelEXT label{};
    label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    label.pLabelName = labelName;
    std::memcpy(label.color, color.data(), sizeof(float) * color.size());
    beginLabel(commandBuffer, &label);
}

void endDebugLabel(PFN_vkCmdEndDebugUtilsLabelEXT endLabel, VkCommandBuffer commandBuffer) {
    if (endLabel != nullptr) {
        endLabel(commandBuffer);
    }
}

void fillInputData(std::vector<uint32_t>& indexData, std::vector<uint32_t>& valueData) {
    indexData.resize(kElementCount);
    valueData.resize(kElementCount);

    for (uint32_t i = 0; i < kElementCount; ++i) {
        indexData[i] = (i * 167U + 31U) % kElementCount;
        valueData[i] = (i * 13U) ^ 0x9e3779b9U;
    }
}

double timestampDeltaNanoseconds(
    uint64_t start,
    uint64_t end,
    const VkPhysicalDeviceProperties& properties) {
    return static_cast<double>(end - start) * static_cast<double>(properties.limits.timestampPeriod);
}
}  // namespace

std::string ComputeLatencyExperiment::name() const {
    return "compute-latency";
}

ExperimentRequirements ComputeLatencyExperiment::requirements() const {
    return {};
}

void ComputeLatencyExperiment::setup(VulkanContext& context) {
    (void)context;
}

void ComputeLatencyExperiment::run(VulkanContext& context) {
    const VkDevice device = context.logicalDevice().handle();
    const VkPhysicalDevice physicalDevice = context.physicalDevice().handle();
    const VkQueue computeQueue = context.logicalDevice().computeQueue();

    const BufferResource indexBuffer(
        physicalDevice,
        device,
        sizeof(uint32_t) * kElementCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    const BufferResource valueBuffer(
        physicalDevice,
        device,
        sizeof(uint32_t) * kElementCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    const BufferResource highLatencyOutputBuffer(
        physicalDevice,
        device,
        sizeof(uint32_t) * kElementCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    const BufferResource highOccupancyOutputBuffer(
        physicalDevice,
        device,
        sizeof(uint32_t) * kElementCount,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    std::vector<uint32_t> indexData;
    std::vector<uint32_t> valueData;
    fillInputData(indexData, valueData);
    indexBuffer.write(indexData.data(), indexData.size() * sizeof(uint32_t));
    valueBuffer.write(valueData.data(), valueData.size() * sizeof(uint32_t));

    const VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(device);
    const VkDescriptorPool descriptorPool = createDescriptorPool(device);
    const VkPipelineLayout pipelineLayout = createPipelineLayout(device, descriptorSetLayout);

    const std::string shaderRoot = VULKAN_EXPERIMENT_SHADER_DIR;
    const VkPipeline highLatencyPipeline =
        createComputePipeline(device, pipelineLayout, shaderRoot + "/high_latency.comp.spv");
    const VkPipeline highOccupancyPipeline =
        createComputePipeline(device, pipelineLayout, shaderRoot + "/high_occupancy.comp.spv");

    const VkDescriptorSet highLatencySet = allocateDescriptorSet(device, descriptorPool, descriptorSetLayout);
    const VkDescriptorSet highOccupancySet = allocateDescriptorSet(device, descriptorPool, descriptorSetLayout);
    updateDescriptorSet(device, highLatencySet, indexBuffer, valueBuffer, highLatencyOutputBuffer);
    updateDescriptorSet(device, highOccupancySet, indexBuffer, valueBuffer, highOccupancyOutputBuffer);

    const VkCommandBuffer commandBuffer =
        allocateCommandBuffer(device, context.computeCommandPool().handle());
    const VkFence fence = createFence(device);
    const VkQueryPool queryPool = createTimestampQueryPool(device);
    const auto beginLabel = getBeginDebugLabel(device);
    const auto endLabel = getEndDebugLabel(device);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin compute experiment command buffer.");
    }

    vkCmdResetQueryPool(commandBuffer, queryPool, 0, 4);

    const PushConstants latencyConstants{kElementCount, kPointerChaseIterations};
    const PushConstants occupancyConstants{kElementCount, kOccupancyIterations};

    beginDebugLabel(beginLabel, commandBuffer, "High Latency Dispatch", {0.85F, 0.33F, 0.10F, 1.0F});
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 0);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, highLatencyPipeline);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout,
        0,
        1,
        &highLatencySet,
        0,
        nullptr);
    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(PushConstants),
        &latencyConstants);
    vkCmdDispatch(commandBuffer, kElementCount / kHighLatencyLocalSize, 1, 1);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 1);
    endDebugLabel(endLabel, commandBuffer);

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

    beginDebugLabel(beginLabel, commandBuffer, "High Occupancy Dispatch", {0.15F, 0.65F, 0.98F, 1.0F});
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 2);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, highOccupancyPipeline);
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        pipelineLayout,
        0,
        1,
        &highOccupancySet,
        0,
        nullptr);
    vkCmdPushConstants(
        commandBuffer,
        pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(PushConstants),
        &occupancyConstants);
    vkCmdDispatch(commandBuffer, kElementCount / kHighOccupancyLocalSize, 1, 1);
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, queryPool, 3);
    endDebugLabel(endLabel, commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record compute experiment command buffer.");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(computeQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit compute experiment command buffer.");
    }

    if (vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        throw std::runtime_error("Failed to wait for compute experiment completion.");
    }

    std::array<uint64_t, 4> timestamps{};
    if (vkGetQueryPoolResults(
            device,
            queryPool,
            0,
            static_cast<uint32_t>(timestamps.size()),
            sizeof(timestamps),
            timestamps.data(),
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT) != VK_SUCCESS) {
        throw std::runtime_error("Failed to read compute experiment timestamps.");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    const double highLatencyNs = timestampDeltaNanoseconds(timestamps[0], timestamps[1], properties);
    const double highOccupancyNs = timestampDeltaNanoseconds(timestamps[2], timestamps[3], properties);

    const auto highLatencyResults = highLatencyOutputBuffer.readUint32(4);
    const auto highOccupancyResults = highOccupancyOutputBuffer.readUint32(4);

    std::cout
        << "RenderDoc comparison target is ready.\n"
        << "Capture this run and compare the two dispatches:\n"
        << "  1. High Latency Dispatch\n"
        << "  2. High Occupancy Dispatch\n"
        << "GPU timings:\n"
        << "  high-latency: " << (highLatencyNs / 1000000.0) << " ms\n"
        << "  high-occupancy: " << (highOccupancyNs / 1000000.0) << " ms\n"
        << "Output samples:\n"
        << "  high-latency[0..3]: "
        << highLatencyResults[0] << ", "
        << highLatencyResults[1] << ", "
        << highLatencyResults[2] << ", "
        << highLatencyResults[3] << '\n'
        << "  high-occupancy[0..3]: "
        << highOccupancyResults[0] << ", "
        << highOccupancyResults[1] << ", "
        << highOccupancyResults[2] << ", "
        << highOccupancyResults[3] << '\n';

    vkDestroyQueryPool(device, queryPool, nullptr);
    vkDestroyFence(device, fence, nullptr);
    vkFreeCommandBuffers(device, context.computeCommandPool().handle(), 1, &commandBuffer);
    vkDestroyPipeline(device, highOccupancyPipeline, nullptr);
    vkDestroyPipeline(device, highLatencyPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void ComputeLatencyExperiment::teardown(VulkanContext& context) {
    context.waitIdle();
}
