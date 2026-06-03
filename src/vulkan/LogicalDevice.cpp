#include "vulkan/LogicalDevice.h"

#include <set>
#include <stdexcept>
#include <vector>

LogicalDevice::LogicalDevice(
    const PhysicalDevice& physicalDevice,
    const std::vector<const char*>& requiredExtensions) {
    const QueueFamilyIndices& indices = physicalDevice.queueFamilies();
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.computeFamily.value(),
    };

    if (indices.presentFamily.has_value()) {
        uniqueQueueFamilies.insert(indices.presentFamily.value());
    }

    constexpr float queuePriority = 1.0F;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (vkCreateDevice(physicalDevice.handle(), &createInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device.");
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.computeFamily.value(), 0, &computeQueue_);

    if (indices.presentFamily.has_value()) {
        vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
    }
}

LogicalDevice::~LogicalDevice() {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
    }
}

VkDevice LogicalDevice::handle() const {
    return device_;
}

VkQueue LogicalDevice::graphicsQueue() const {
    return graphicsQueue_;
}

VkQueue LogicalDevice::presentQueue() const {
    return presentQueue_;
}

VkQueue LogicalDevice::computeQueue() const {
    return computeQueue_;
}
