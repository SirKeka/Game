#pragma once

#include "renderer/renderer_types.hpp"

#include "core/asserts.hpp"
#include "vulkan_image.hpp"
#include "vulkan_framebuffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_buffer.hpp"
#include "shaders/vulkan_material_shader.hpp"

struct VulkanSwapchain 
{
    VkSurfaceFormatKHR ImageFormat;
    u8 MaxFramesInFlight;
    VkSwapchainKHR handle;
    u32 ImageCount;
    VkImage* images;
    VkImageView* views;

    VulkanImage* DepthAttachment;

    // Буферы кадров, используемые для экранного рендеринга.
    DArray<VulkanFramebuffer> framebuffers;
};

struct VulkanFence {
    VkFence handle;
    bool IsSignaled;
};

// Проверяет возвращаемое значение данного выражения на соответствие VK_SUCCESS.
#define VK_CHECK(expr)           \
{                                \
    MASSERT(expr == VK_SUCCESS); \
}

class VulkanAPI : public RendererType
{
public:
    f32 FrameDeltaTime;

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

    VulkanDevice Device{};

    VulkanSwapchain swapchain{};
    VulkanRenderPass MainRenderpass{};

    VulkanBuffer ObjectVertexBuffer;
    VulkanBuffer ObjectIndexBuffer;

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

    VulkanMaterialShader MaterialShader;

    u64 GeometryVertexOffset;
    u64 GeometryIndexOffset;

public:
    VulkanAPI() {}
    ~VulkanAPI();
    
    bool Initialize(MWindow* window, const char* ApplicationName) override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    void UpdateGlobalState(const Matrix4D& projection, const Matrix4D& view, const Vector3D<f32>& ViewPosition, const Vector4D<f32>& AmbientColour, i32 mode) override;
    bool EndFrame(f32 DeltaTime) override;
    bool CreateMaterial(class Material* material) override;
    void DestroyMaterial(class Material* material) override;

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

    void UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer& buffer, u64 offset, u64 size, void* data);
    void DrawGeometry(const GeometryRenderData& data) override;
};
