#pragma once

#include "vulkan_api.hpp"

//TODO: Подумать об ООП

void VulkanSwapchainCreate(
    VulkanAPI* VkAPI,
    u32 width,
    u32 height,
    VulkanSwapchain* OutSwapchain);

void VulkanSwapchainRecreate(
    VulkanAPI* VkAPI,
    u32 width,
    u32 height,
    VulkanSwapchain* swapchain);

void VulkanSwapchainDestroy(
    VulkanAPI* VkAPI,
    VulkanSwapchain* swapchain);

bool VulkanSwapchainAcquireNextImageIndex(
    VulkanAPI* VkAPI,
    VulkanSwapchain* swapchain,
    u64 TimeoutNs,
    VkSemaphore ImageAvailableSemaphore,
    VkFence fence,
    u32* OutImageIndex);

void VulkanSwapchainPresent(
    VulkanAPI* VkAPI,
    VulkanSwapchain* swapchain,
    VkQueue GraphicsQueue,
    VkQueue PresentQueue,
    VkSemaphore RenderCompleteSemaphore,
    u32 PresentImageIndex);