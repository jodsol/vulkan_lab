#include "vulkan/PhysicalDevice.h"

#include <cstring>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, QueueFamilyIndices& outQueueFamilies) {
    outQueueFamilies = findQueueFamilies(device, surface);
    if (!outQueueFamilies.isComplete() || !checkDeviceExtensionSupport(device)) {
        return false;
    }

    const auto swapchainSupport = querySwapchainSupport(device, surface);
    return !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
}
}  // namespace

PhysicalDevice::PhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& candidate : devices) {
        QueueFamilyIndices candidateQueues;
        if (isDeviceSuitable(candidate, surface, candidateQueues)) {
            device_ = candidate;
            queueFamilies_ = candidateQueues;
            break;
        }
    }

    if (device_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU.");
    }
}

VkPhysicalDevice PhysicalDevice::handle() const {
    return device_;
}

const QueueFamilyIndices& PhysicalDevice::queueFamilies() const {
    return queueFamilies_;
}

SwapchainSupportDetails PhysicalDevice::querySwapchainSupport(VkSurfaceKHR surface) const {
    return ::querySwapchainSupport(device_, surface);
}
