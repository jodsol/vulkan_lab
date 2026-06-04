#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

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
        VkMemoryPropertyFlags properties);

    BufferResource(const BufferResource&) = delete;
    BufferResource& operator=(const BufferResource&) = delete;
    BufferResource(BufferResource&& other) noexcept;
    BufferResource& operator=(BufferResource&& other) noexcept;
    ~BufferResource();

    void write(const void* data, size_t dataSize) const;
    std::vector<uint32_t> readUint32(size_t count) const;
    std::vector<float> readFloat(size_t count) const;

private:
    void destroy();
};

std::vector<char> readBinaryFile(const std::string& path);
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
VkPipeline createComputePipeline(
    VkDevice device,
    VkPipelineLayout pipelineLayout,
    const std::string& shaderPath);
VkDescriptorSetLayout createDescriptorSetLayout(
    VkDevice device,
    uint32_t bindingCount,
    VkDescriptorType descriptorType,
    VkShaderStageFlags stageFlags);
VkDescriptorPool createDescriptorPool(
    VkDevice device,
    VkDescriptorType descriptorType,
    uint32_t descriptorCount,
    uint32_t maxSets);
VkPipelineLayout createPipelineLayout(
    VkDevice device,
    VkDescriptorSetLayout descriptorSetLayout,
    uint32_t pushConstantSize);
VkCommandBuffer allocateCommandBuffer(VkDevice device, VkCommandPool commandPool);
VkFence createFence(VkDevice device);
VkQueryPool createTimestampQueryPool(VkDevice device, uint32_t queryCount);
VkDescriptorSet allocateDescriptorSet(
    VkDevice device,
    VkDescriptorPool descriptorPool,
    VkDescriptorSetLayout descriptorSetLayout);
void updateStorageBufferDescriptorSet(
    VkDevice device,
    VkDescriptorSet descriptorSet,
    const std::vector<const BufferResource*>& buffers);
PFN_vkCmdBeginDebugUtilsLabelEXT getBeginDebugLabel(VkDevice device);
PFN_vkCmdEndDebugUtilsLabelEXT getEndDebugLabel(VkDevice device);
void beginDebugLabel(
    PFN_vkCmdBeginDebugUtilsLabelEXT beginLabel,
    VkCommandBuffer commandBuffer,
    const char* labelName,
    const std::array<float, 4>& color);
void endDebugLabel(PFN_vkCmdEndDebugUtilsLabelEXT endLabel, VkCommandBuffer commandBuffer);
double timestampDeltaNanoseconds(
    uint64_t start,
    uint64_t end,
    const VkPhysicalDeviceProperties& properties);
