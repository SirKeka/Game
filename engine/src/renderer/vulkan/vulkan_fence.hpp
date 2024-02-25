#pragma once

#include "vulkan_api.hpp"

void VulkanFenceCreate(
    VulkanAPI* VkAPI,
    bool CreateSignaled,
    VulkanFence* OutFence);

void VulkanFenceDestroy(VulkanAPI* VkAPI, VulkanFence* fence);

bool VulkanFenceWait(VulkanAPI* VkAPI, VulkanFence* fence, u64 TimeoutNs);

void VulkanFenceReset(VulkanAPI* VkAPI, VulkanFence* fence);