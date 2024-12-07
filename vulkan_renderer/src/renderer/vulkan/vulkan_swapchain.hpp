#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>
#include "renderer/renderpass.hpp"
//#include "renderer/renderer_structs.hpp"

class VulkanAPI;
struct Texture;
using RendererConfigFlags = u32;

class VulkanSwapchain
{
public:
    VkSurfaceFormatKHR ImageFormat; // Формат изображения цепочки обмена.
    u8 MaxFramesInFlight;           // Максимальное количество «изображений в полете» (изображений, которые одновременно визуализируются). Обычно на одно меньше общего количества доступных изображений.
    RendererConfigFlags flags;      // Указывает различные флаги, используемые для создания экземпляра цепочки обмена.
    VkSwapchainKHR handle;          // Внутренний дескриптор цепочки обмена.
    u32 ImageCount;                 // Количество изображений цепочки обмена.
    Texture* RenderTextures;        // Массив целей рендеринга, которые содержат изображения цепочки обмена.
    Texture* DepthTextures;         // Массив текстур глубины на один кадр.
    RenderTarget RenderTargets[3];  // Цели рендеринга, используемые для экранного рендеринга, по одной на кадр. Изображения, содержащиеся в них, создаются и принадлежат цепочке обмена.
public:
    constexpr VulkanSwapchain() : ImageFormat(), MaxFramesInFlight(), flags(), handle(), ImageCount(), RenderTextures(nullptr), DepthTextures(nullptr), RenderTargets() {}
    VulkanSwapchain(VulkanAPI* VkAPI, u32 width, u32 height, RendererConfigFlags flags);
    ~VulkanSwapchain();

    void Create(VulkanAPI* VkAPI, u32 width, u32 height, RendererConfigFlags flags);
    void Destroy(VulkanAPI* VkAPI);

    void Recreate(VulkanAPI* VkAPI, u32 width, u32 height);

    bool AcquireNextImageIndex(
        VulkanAPI* VkAPI,
        u64 TimeoutNs,
        VkSemaphore ImageAvailableSemaphore,
        VkFence fence,
        u32* OutImageIndex);

    /// @brief 
    /// @param VkAPI 
    /// @param PresentQueue 
    /// @param RenderCompleteSemaphore 
    /// @param PresentImageIndex 
    void Present(
        VulkanAPI* VkAPI,
        VkQueue PresentQueue,
        VkSemaphore RenderCompleteSemaphore,
        u32 PresentImageIndex);
};
