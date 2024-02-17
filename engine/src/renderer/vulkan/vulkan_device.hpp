#pragma once

#include "renderer/vulkan/vulkan_api.hpp"

bool VulkanDeviceCreate();

void VulkanDeviceDestroy();

void VulkanDeviceQuerySwapchainSupport(
    VkPhysicalDevice PhysicalDevice,
    VkSurfaceKHR Surface,
    VulkanSwapchainSupportInfo* OutSupportInfo);