#pragma once

#include "vulkan_api.hpp"

class VulkanDevice
{
public:
    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
    VulkanSwapchainSupportInfo SwapchainSupport{};
    i32 GraphicsQueueIndex;
    i32 PresentQueueIndex;
    i32 TransferQueueIndex;

    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkQueue TransferQueue;
    
    VkCommandPool GraphicsCommandPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat DepthFormat;
public:
    VulkanDevice() = default;
    ~VulkanDevice();

    bool Create(VulkanAPI* VkAPI);

    void Destroy(VulkanAPI* VkAPI);

    void QuerySwapchainSupport(
        VkPhysicalDevice PhysicalDevice,
        VkSurfaceKHR Surface,
        VulkanSwapchainSupportInfo* OutSupportInfo);

    bool DetectDepthFormat(VulkanDevice* Device);

private:

    struct VulkanPhysicalDeviceRequirements {
    bool graphics;
    bool present;
    bool compute;
    bool transfer;
    DArray<const char*> DeviceExtensionNames;
    bool SamplerAnisotropy;
    bool DiscreteGPU;
    };

    struct VulkanPhysicalDeviceQueueFamilyInfo {
        u32 GraphicsFamilyIndex;
        u32 PresentFamilyIndex;
        u32 ComputeFamilyIndex;
        u32 TransferFamilyIndex;
    };

    bool SelectPhysicalDevice(VulkanAPI* VkAPI);
    bool PhysicalDeviceMeetsRequirements(
        VkPhysicalDevice device,
        VkSurfaceKHR surface,
        const VkPhysicalDeviceProperties* properties,
        const VkPhysicalDeviceFeatures* features,
        const VulkanPhysicalDeviceRequirements* requirements,
        VulkanPhysicalDeviceQueueFamilyInfo* OutQueueFamilyInfo,
        VulkanSwapchainSupportInfo* OutSwapchainSupport
    );


};
