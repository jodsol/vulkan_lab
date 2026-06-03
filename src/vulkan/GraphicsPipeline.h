#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class GraphicsPipeline {
public:
    GraphicsPipeline(
        VkDevice device,
        VkExtent2D swapchainExtent,
        VkRenderPass renderPass,
        const std::vector<char>& vertexShaderCode,
        const std::vector<char>& fragmentShaderCode);
    ~GraphicsPipeline();

    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    VkPipelineLayout layout() const;
    VkPipeline handle() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;

    VkShaderModule createShaderModule(const std::vector<char>& code) const;
};
