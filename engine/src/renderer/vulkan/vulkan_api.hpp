#pragma once

#include "renderer/renderer_types.hpp"

#include "core/asserts.hpp"
#include "containers\darray.hpp"

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
    
    VkCommandPool GraphicsCommandPool;

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

enum VulkanRenderPassState {
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

struct VulkanRenderpass {
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
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

enum VulkanCommandBufferState {
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
};

struct VulkanCommandBuffer {
    VkCommandBuffer handle;

    // Состояние буфера команд.
    VulkanCommandBufferState state;
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
    VulkanRenderpass MainRenderpass{};

    DArray<VulkanCommandBuffer> GraphicsCommandBuffers;

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

private:
    void CreateCommandBuffers();
};
