#pragma once

#include "renderer/renderer_types.hpp"
#include "core/asserts.hpp"

#include <vulkan/vulkan.h>

#include "platform/platform.hpp"

struct VulkanSwapchainSupportInfo 
{
    VkSurfaceCapabilitiesKHR capabilities;
    u32 FormatCount;
    VkSurfaceFormatKHR* formats;
    u32 PresentModeCount;
    VkPresentModeKHR* PresentModes;
};

struct VulkanDevice
{
    VkPhysicalDevice PhysicalDevice;
    VkDevice LogicalDevice;
    VulkanSwapchainSupportInfo SwapchainSupport;
    i32 GraphicsQueueIndex;
    i32 PresentQueueIndex;
    i32 TransferQueueIndex;

    VkQueue GraphicsQueue;
    VkQueue PresentQueue;
    VkQueue TransferQueue;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

    VkFormat DepthFormat;
};

struct VulkanImage {
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
};

struct VulkanSwapchain {
    VkSurfaceFormatKHR ImageFormat;
    u8 MaxFramesInFlight;
    VkSwapchainKHR handle;
    u32 ImageCount;
    VkImage* images;
    VkImageView* views;

    VulkanImage DepthAttachment;
};

// Проверяет возвращаемое значение данного выражения на соответствие VK_SUCCESS.
#define VK_CHECK(expr)           \
{                                \
    MASSERT(expr == VK_SUCCESS); \
}

class VulkanAPI : public RendererType
{
public:
    // Текущая ширина фреймбуфера.
    u32 FramebufferWidth{0};

    // Текущая высота фреймбуфера.
    u32 FramebufferHeight{0};

    static VkInstance instance;
    static VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface{};

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    VulkanDevice Device{};

    VulkanSwapchain swapchain{};
    u32 ImageIndex{0};
    u32 CurrentFrame{0};

    bool RecreatingSwapchain;

public:
    VulkanAPI() {};
    ~VulkanAPI();
    
    bool Initialize(MWindow* window, const char* ApplicationName) override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    bool EndFrame(f32 DeltaTime) override;

    void* operator new(u64 size);
    void operator delete(void* ptr);

    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);
};
