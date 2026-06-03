#include "vulkan/PhysicalDevice.h"

#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
bool checkDeviceExtensionSupport(
    VkPhysicalDevice device,
    const std::vector<const char*>& requiredExtensions) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> remainingExtensions(requiredExtensions.begin(), requiredExtensions.end());
    for (const auto& extension : availableExtensions) {
        remainingExtensions.erase(extension.extensionName);
    }

    return remainingExtensions.empty();
}

bool isDeviceSuitable(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const std::vector<const char*>& requiredExtensions,
    bool requiresPresent,
    QueueFamilyIndices& outQueueFamilies) {
    outQueueFamilies = findQueueFamilies(device, surface, requiresPresent);
    if (!outQueueFamilies.isComplete(requiresPresent)) {
        return false;
    }

    if (!checkDeviceExtensionSupport(device, requiredExtensions)) {
        return false;
    }

    if (!requiresPresent) {
        return true;
    }

    const auto swapchainSupport = querySwapchainSupport(device, surface);
    return !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
}
}  // namespace

PhysicalDevice::PhysicalDevice(
    VkInstance instance,
    VkSurfaceKHR surface,
    const std::vector<const char*>& requiredExtensions,
    bool requiresPresent) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& candidate : devices) {
        QueueFamilyIndices candidateQueues;
        if (isDeviceSuitable(candidate, surface, requiredExtensions, requiresPresent, candidateQueues)) {
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
