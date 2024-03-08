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

class VulkanDevice;

struct VulkanImage 
{
    VkImage handle;
    VkDeviceMemory memory;
    VkImageView view;
    u32 width;
    u32 height;
};

enum VulkanRenderPassState 
{
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

struct VulkanRenderpass 
{
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
};

struct VulkanFramebuffer
{
    VkFramebuffer handle;
    u32 AttachmentCount;
    VkImageView* attachments;
    VulkanRenderpass* renderpass;
};

struct VulkanSwapchain 
{
    VkSurfaceFormatKHR ImageFormat;
    u8 MaxFramesInFlight;
    VkSwapchainKHR handle;
    u32 ImageCount;
    VkImage* images;
    VkImageView* views;

    VulkanImage DepthAttachment;

    // Буферы кадров, используемые для экранного рендеринга.
    DArray<VulkanFramebuffer> framebuffers;
};

enum VulkanCommandBufferState 
{
    COMMAND_BUFFER_STATE_READY,
    COMMAND_BUFFER_STATE_RECORDING,
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
};

struct VulkanCommandBuffer 
{
    VkCommandBuffer handle;

    // Состояние буфера команд.
    VulkanCommandBufferState state;
};

struct VulkanFence {
    VkFence handle;
    bool IsSignaled;
};

class VulkanObjectShader;

// Проверяет возвращаемое значение данного выражения на соответствие VK_SUCCESS.
#define VK_CHECK(expr)           \
{                                \
    MASSERT(expr == VK_SUCCESS); \
}

class VulkanBuffer;

class VulkanAPI : public RendererType
{
using LinearAllocator = WrapLinearAllocator<VulkanAPI>;

public:
    // Текущая ширина фреймбуфера.
    u32 FramebufferWidth{0};

    // Текущая высота фреймбуфера.
    u32 FramebufferHeight{0};

    // Текущее поколение размера кадрового буфера. Если он не соответствует 
    // FramebufferSizeLastGeneration, необходимо создать новый.
    u64 FramebufferSizeGeneration{0};

    // Генерация кадрового буфера при его последнем создании. 
    // При обновлении установите значение FramebufferSizeGeneration.
    u64 FramebufferSizeLastGeneration{0};

    static VkInstance instance;
    static VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface{};

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    VulkanDevice* Device;

    VulkanSwapchain swapchain{};
    VulkanRenderpass MainRenderpass{};

    VulkanBuffer* ObjectVertexBuffer;
    VulkanBuffer* ObjectIndexBuffer;

    DArray<VulkanCommandBuffer> GraphicsCommandBuffers;
    DArray<VkSemaphore> ImageAvailableSemaphores;
    DArray<VkSemaphore> QueueCompleteSemaphores;

    u32 InFlightFenceCount;
    DArray<VulkanFence> InFlightFences;

    // Содержит указатели на заборы, которые существуют и находятся в собственности в другом месте.
    DArray<VulkanFence*> ImagesInFlight;

    u32 ImageIndex{0};
    u32 CurrentFrame{0};

    bool RecreatingSwapchain{false};

    VulkanObjectShader* ObjectShader;

    u64 GeometryVertexOffset;
    u64 GeometryIndexOffset;

public:
    VulkanAPI() {}
    ~VulkanAPI();
    
    bool Initialize(MWindow* window, const char* ApplicationName) override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    bool EndFrame(f32 DeltaTime) override;

    void* operator new(u64 size);
    //void operator delete(void* ptr);

    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);

private:
    static u32 CachedFramebufferWidth;
    static u32 CachedFramebufferHeight;

private:
    void CreateCommandBuffers();
    void RegenerateFramebuffers();
    bool RecreateSwapchain();
    bool CreateBuffers();
};
