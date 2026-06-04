#include "experiments/core/ComputeSupport.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace {
uint32_t findMemoryTypeIndex(
    VkPhysicalDevice physicalDevice,
    uint32_t typeBits,
    VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        const bool typeMatches = (typeBits & (1U << i)) != 0;
        const bool propertiesMatch =
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;

        if (typeMatches && propertiesMatch) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find a suitable buffer memory type.");
}
}  // namespace

BufferResource::BufferResource(
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

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex =
        findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate compute experiment buffer memory.");
    }

    if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
        throw std::runtime_error("Failed to bind compute experiment buffer memory.");
    }
}

BufferResource::BufferResource(BufferResource&& other) noexcept
    : device(other.device),
      buffer(other.buffer),
      memory(other.memory),
      size(other.size) {
    other.device = VK_NULL_HANDLE;
    other.buffer = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
    other.size = 0;
}

BufferResource& BufferResource::operator=(BufferResource&& other) noexcept {
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

BufferResource::~BufferResource() {
    destroy();
}

void BufferResource::write(const void* data, size_t dataSize) const {
    void* mapped = nullptr;
    if (vkMapMemory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map compute experiment buffer.");
    }

    std::memcpy(mapped, data, dataSize);
    vkUnmapMemory(device, memory);
}

std::vector<uint32_t> BufferResource::readUint32(size_t count) const {
    void* mapped = nullptr;
    if (vkMapMemory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map compute output buffer.");
    }

    std::vector<uint32_t> values(count);
    std::memcpy(values.data(), mapped, count * sizeof(uint32_t));
    vkUnmapMemory(device, memory);
    return values;
}

std::vector<float> BufferResource::readFloat(size_t count) const {
    void* mapped = nullptr;
    if (vkMapMemory(device, memory, 0, size, 0, &mapped) != VK_SUCCESS) {
        throw std::runtime_error("Failed to map compute output buffer.");
    }

    std::vector<float> values(count);
    std::memcpy(values.data(), mapped, count * sizeof(float));
    vkUnmapMemory(device, memory);
    return values;
}

void BufferResource::destroy() {
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

VkDescriptorSetLayout createDescriptorSetLayout(
    VkDevice device,
    uint32_t bindingCount,
    VkDescriptorType descriptorType,
    VkShaderStageFlags stageFlags) {
    std::vector<VkDescriptorSetLayoutBinding> bindings(bindingCount);
    for (uint32_t i = 0; i < bindingCount; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorType = descriptorType;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = stageFlags;
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

VkDescriptorPool createDescriptorPool(
    VkDevice device,
    VkDescriptorType descriptorType,
    uint32_t descriptorCount,
    uint32_t maxSets) {
    VkDescriptorPoolSize poolSize{};
    poolSize.type = descriptorType;
    poolSize.descriptorCount = descriptorCount;

    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = 1;
    createInfo.pPoolSizes = &poolSize;
    createInfo.maxSets = maxSets;

    VkDescriptorPool pool = VK_NULL_HANDLE;
    if (vkCreateDescriptorPool(device, &createInfo, nullptr, &pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool.");
    }

    return pool;
}

VkPipelineLayout createPipelineLayout(
    VkDevice device,
    VkDescriptorSetLayout descriptorSetLayout,
    uint32_t pushConstantSize) {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = pushConstantSize;

    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.setLayoutCount = 1;
    createInfo.pSetLayouts = &descriptorSetLayout;
    createInfo.pushConstantRangeCount = pushConstantSize > 0 ? 1U : 0U;
    createInfo.pPushConstantRanges = pushConstantSize > 0 ? &pushConstantRange : nullptr;

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

VkQueryPool createTimestampQueryPool(VkDevice device, uint32_t queryCount) {
    VkQueryPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = queryCount;

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

void updateStorageBufferDescriptorSet(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    const std::vector<const BufferResource*>& buffers) {
    std::vector<VkDescriptorBufferInfo> bufferInfos(buffers.size());
    std::vector<VkWriteDescriptorSet> writes(buffers.size());

    for (size_t i = 0; i < buffers.size(); ++i) {
        bufferInfos[i].buffer = buffers[i]->buffer;
        bufferInfos[i].offset = 0;
        bufferInfos[i].range = buffers[i]->size;

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = descriptorSet;
        writes[i].dstBinding = static_cast<uint32_t>(i);
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &bufferInfos[i];
    }

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

double timestampDeltaNanoseconds(
    uint64_t start,
    uint64_t end,
    const VkPhysicalDeviceProperties& properties) {
    return static_cast<double>(end - start) * static_cast<double>(properties.limits.timestampPeriod);
}
