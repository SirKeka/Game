#pragma once

#include "renderer/renderer_types.hpp"

#include "core/asserts.hpp"
#include "vulkan_image.hpp"
#include "vulkan_device.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_buffer.hpp"
#include "shaders/vulkan_material_shader.hpp"
#include "shaders/vulkan_ui_shader.hpp"
#include "resources/geometry.hpp"
#include "math/vertex.hpp"

struct VulkanSwapchain 
{
    VkSurfaceFormatKHR ImageFormat;
    u8 MaxFramesInFlight;
    VkSwapchainKHR handle;
    u32 ImageCount;
    VkImage* images;
    VkImageView* views;

    VulkanImage* DepthAttachment;

    // Буферы кадров, используемые для экранного рендеринга, по три на кадр.
    VkFramebuffer framebuffers[3];
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

    VkInstance instance;
    VkAllocationCallbacks* allocator;
    VkSurfaceKHR surface{};

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;
#endif

    VulkanDevice Device{};

    VulkanSwapchain swapchain{};
    VulkanRenderPass MainRenderpass{};
    VulkanRenderPass UI_Renderpass;

    VulkanBuffer ObjectVertexBuffer;
    VulkanBuffer ObjectIndexBuffer;

    DArray<VulkanCommandBuffer> GraphicsCommandBuffers;
    DArray<VkSemaphore> ImageAvailableSemaphores;
    DArray<VkSemaphore> QueueCompleteSemaphores;

    u32 InFlightFenceCount;
    VkFence InFlightFences[2];

    // Содержит указатели на заборы, которые существуют и находятся в собственности в другом месте, по одному на кадр.
    VkFence* ImagesInFlight[3];

    u32 ImageIndex{0};
    u32 CurrentFrame{0};

    bool RecreatingSwapchain{false};

    VulkanMaterialShader MaterialShader;
    VulkanUI_Shader UI_Shader;

    u64 GeometryVertexOffset;
    u64 GeometryIndexOffset;

    // TODO: сделать динамическим, копии геометрий хранятся в системе геометрий, возможно стоит хранить здесь указатели на геометрии
    Geometry geometries[VULKAN_MAX_GEOMETRY_COUNT]{};

    // Буферы кадров, используемые для рендеринга мира, по одному на кадр.
    VkFramebuffer WorldFramebuffers[3];

private:
    u32 CachedFramebufferWidth;
    u32 CachedFramebufferHeight;

public:
    VulkanAPI() = default;
    ~VulkanAPI();
    
    bool Initialize(MWindow* window, const char* ApplicationName) override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    void UpdateGlobalWorldState(const Matrix4D& projection, const Matrix4D& view, const Vector3D<f32>& ViewPosition, const Vector4D<f32>& AmbientColour, i32 mode) override;
    void UpdateGlobalUIState(const Matrix4D& projection, const Matrix4D& view, i32 mode) override;
    bool EndFrame(f32 DeltaTime) override;
    bool BeginRenderpass(u8 RenderpassID) override;
    bool EndRenderpass(u8 RenderpassID) override;
    bool CreateMaterial(class Material* material) override;
    void DestroyMaterial(class Material* material) override;
    // TODO: перенести в класс системы визуализации
    bool Load(GeometryID* gid, u32 VertexCount, const Vertex3D* vertices, u32 IndexCount, const u32* indices) override;
    void Unload(GeometryID* gid) override;
    void DrawGeometry(const GeometryRenderData& data) override;

    // void* operator new(u64 size);

    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);

private:
    void CreateCommandBuffers();
    void RegenerateFramebuffers();
    bool RecreateSwapchain();
    bool CreateBuffers();

    void UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer& buffer, u64 offset, u64 size, const void* data);
    void FreeDataRange(VulkanBuffer* buffer, u64 offset, u64 size);
    
};
