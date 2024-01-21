//
// Created by WeslyChen on 2024/1/21.
//

#include "VulkanDeviceWrapper.h"

#include <map>
#include <vector>

#include "Engine/EngineCore.h"

using namespace std;

void VulkanDeviceWrapper::InitDevice(VkInstance instance)
{
    if (!instance)
        return;
    mInstance = instance;

    // Get device
    uint32 deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
        return Logger::LogFatal("VulkanRender", "Failed to find GPUs with Vulkan support!");
    vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int32, VkPhysicalDevice> candidates;
    for (const auto& device : devices)
    {
        int32 score = ScoreDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    // Check if the best candidate is suitable at all
    if (candidates.rbegin()->first > 0)
        mDevice = candidates.rbegin()->second;
    else
        Logger::LogFatal("VulkanRender", "Failed to find a suitable GPU!");
}

int32 VulkanDeviceWrapper::ScoreDeviceSuitability(VkPhysicalDevice device) const
{
    if (!device)
        return 0;

    QueueFamilyIndices indices = FindQueueFamilies(device);
    if (!indices.IsComplete())
        return 0;

    int32 score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader)
        return 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

QueueFamilyIndices VulkanDeviceWrapper::FindQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int32 i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if (indices.IsComplete())
            break;

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;
        i++;
    }

    return indices;
}
