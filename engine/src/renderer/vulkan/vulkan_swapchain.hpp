#pragma once

//#include "vulkan_api.hpp"

class VulkanAPI;

class VulkanSwapchain
{
private:
    VkSurfaceFormatKHR ImageFormat;     // Формат изображения цепочки обмена.
    u8 MaxFramesInFlight;               // Максимальное количество «изображений в полете» (изображений, которые одновременно визуализируются). Обычно на одно меньше общего количества доступных изображений.
    VkSwapchainKHR handle;              // Внутренний дескриптор цепочки обмена.
    u32 ImageCount;                     // Количество изображений цепочки обмена.
    class Texture** RenderTextures;     // Массив указателей для целей рендеринга, которые содержат изображения цепочки обмена.

    class VulkanImage* DepthAttachment; // Вложение изображения глубины.

    VkFramebuffer framebuffers[3];      // Буферы кадров, используемые для экранного рендеринга, по три на кадр.
public:
    VulkanSwapchain(VulkanAPI* VkAPI, u32 width, u32 height);
    ~VulkanSwapchain();

    void Create(VulkanAPI* VkAPI, u32 width, u32 height);

    void VulkanSwapchainRecreate(VulkanAPI* VkAPI, u32 width, u32 height);

    void VulkanSwapchainDestroy(VulkanAPI* VkAPI);

    bool VulkanSwapchainAcquireNextImageIndex(
        VulkanAPI* VkAPI,
        u64 TimeoutNs,
        VkSemaphore ImageAvailableSemaphore,
        VkFence fence,
        u32* OutImageIndex);

    void VulkanSwapchainPresent(
        VulkanAPI* VkAPI,
        VkQueue GraphicsQueue,
        VkQueue PresentQueue,
        VkSemaphore RenderCompleteSemaphore,
        u32 PresentImageIndex);
};
