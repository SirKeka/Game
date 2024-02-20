#pragma once

#include "renderer/vulkan/vulkan_api.hpp"

// TODO: возможно сделать в стиле ООП

bool VulkanDeviceCreate(VulkanAPI* VkAPI);

void VulkanDeviceDestroy(VulkanAPI* VkAPI);

void VulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice PhysicalDevice,
    VkSurfaceKHR Surface,
    VulkanSwapchainSupportInfo* OutSupportInfo);