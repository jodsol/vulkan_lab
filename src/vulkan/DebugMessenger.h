#pragma once

#include <vulkan/vulkan.h>

class DebugMessenger {
public:
    DebugMessenger(VkInstance instance, bool enabled);
    ~DebugMessenger();

    DebugMessenger(const DebugMessenger&) = delete;
    DebugMessenger& operator=(const DebugMessenger&) = delete;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger_ = VK_NULL_HANDLE;
};
