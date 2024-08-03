#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>

class VulkanAPI;
class Texture;

class VulkanSwapchain
{
public:
    VkSurfaceFormatKHR ImageFormat; // Формат изображения цепочки обмена.
    u8 MaxFramesInFlight;           // Максимальное количество «изображений в полете» (изображений, которые одновременно визуализируются). Обычно на одно меньше общего количества доступных изображений.
    VkSwapchainKHR handle;          // Внутренний дескриптор цепочки обмена.
    u32 ImageCount;                 // Количество изображений цепочки обмена.
    Texture** RenderTextures;       // Массив указателей для целей рендеринга, которые содержат изображения цепочки обмена.
    Texture* DepthTexture;          // Глубинная текстура.
    RenderTarget RenderTargets[3];  // Цели рендеринга, используемые для экранного рендеринга, по одной на кадр. Изображения, содержащиеся в них, создаются и принадлежат цепочке обмена.
public:
    constexpr VulkanSwapchain() : ImageFormat(), MaxFramesInFlight(), handle(), ImageCount(), RenderTextures(nullptr), DepthTexture(nullptr), RenderTargets() {}
    VulkanSwapchain(VulkanAPI* VkAPI, u32 width, u32 height);
    ~VulkanSwapchain();

    void Create(VulkanAPI* VkAPI, u32 width, u32 height);
    void Destroy(VulkanAPI* VkAPI);

    void Recreate(VulkanAPI* VkAPI, u32 width, u32 height);

    bool AcquireNextImageIndex(
        VulkanAPI* VkAPI,
        u64 TimeoutNs,
        VkSemaphore ImageAvailableSemaphore,
        VkFence fence,
        u32* OutImageIndex);

    void Present(
        VulkanAPI* VkAPI,
        VkQueue GraphicsQueue,
        VkQueue PresentQueue,
        VkSemaphore RenderCompleteSemaphore,
        u32 PresentImageIndex);
};
