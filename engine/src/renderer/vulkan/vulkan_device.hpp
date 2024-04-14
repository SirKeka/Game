#pragma once

#include <vulkan/vulkan.h>
#include "containers/darray.hpp"

class VulkanAPI;

struct VulkanSwapchainSupportInfo 
{
    VkSurfaceCapabilitiesKHR capabilities;
    u32 FormatCount;
    VkSurfaceFormatKHR* formats;
    u32 PresentModeCount;
    VkPresentModeKHR* PresentModes;
};

class VulkanDevice
{
public:
    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
    VulkanSwapchainSupportInfo SwapchainSupport;
    i32 GraphicsQueueIndex;
    i32 PresentQueueIndex;
    i32 TransferQueueIndex;
    bool SupportsDeviceLocalHostVisible;

    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkQueue TransferQueue;
    
    VkCommandPool GraphicsCommandPool;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat DepthFormat;
public:
    VulkanDevice() : PhysicalDevice(), LogicalDevice(), SwapchainSupport(), GraphicsQueueIndex(), PresentQueueIndex(), TransferQueueIndex(), SupportsDeviceLocalHostVisible(), 
    GraphicsQueue(), PresentQueue(), TransferQueue(), GraphicsCommandPool(), properties(), features(), memory(), DepthFormat() {}
    ~VulkanDevice() = default;

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
