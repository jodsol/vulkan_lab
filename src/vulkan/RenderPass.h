#pragma once

#include <vulkan/vulkan.h>

class RenderPass {
public:
    RenderPass(VkDevice device, VkFormat colorFormat);
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    VkRenderPass handle() const;

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
};
