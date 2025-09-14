#pragma once

#include <core/asserts.hpp>

#include "renderer/renderer_types.h"

#include "vulkan_device.h"
#include "vulkan_swapchain.hpp"
#include "vulkan_command_buffer.hpp"
#include "vulkan_shader.hpp"
#include "vulkan_structs.h"
#include "resources/geometry.h"
#include "math/vertex.h"

// Проверяет возвращаемое значение данного выражения на соответствие VK_SUCCESS.
#define VK_CHECK(expr)           \
{                                \
    MASSERT(expr == VK_SUCCESS); \
}

class VulkanAPI : public RendererPlugin
{
public:
    u32 ApiMajor;                                      // Поддерживаемая основная версия API на уровне устройства.
    u32 ApiMinor;                                       // Поддерживаемая второстепенная версия API на уровне устройства.
    u32 ApiPatch;                                       // Поддерживаемая версия исправления API на уровне устройства.

    f32 FrameDeltaTime{};                               // Время в секундах с момента последнего кадра.
    u32 FramebufferWidth{};                             // Текущая ширина фреймбуфера.
    u32 FramebufferHeight{};                            // Текущая высота фреймбуфера.
    u64 FramebufferSizeGeneration{};                    // Текущее поколение размера кадрового буфера. Если он не соответствует FramebufferSizeLastGeneration, необходимо создать новый.
    u64 FramebufferSizeLastGeneration{};                // Генерация кадрового буфера при его последнем создании. При обновлении установите значение FramebufferSizeGeneration.
    FVec4 ViewportRect;                                 // Прямоугольник области просмотра.
    FVec4 ScissorRect;                                  // Прямоугольник ножниц.
    VkInstance instance;                                // Дескриптор внутреннего экземпляра Vulkan.
    VkAllocationCallbacks* allocator{nullptr};          // Внутренний распределитель Vulkan.
    VkSurfaceKHR surface;                               // Внутренняя поверхность Vulkan, на которой будет отображаться окно.

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;            // Мессенджер отладки, если он активен.
    /// @brief Указатель функции для установки имен объектов отладки.
    PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;

    /// @brief Указатель функции для установки данных тега объекта отладки в свободной форме.
    PFN_vkSetDebugUtilsObjectTagEXT pfnSetDebugUtilsObjectTagEXT;

    PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
    PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
#endif

    VulkanDevice Device;                                // Устройство Vulkan.
    VulkanSwapchain swapchain;                          // Цепочка подкачки.
    RenderBuffer ObjectVertexBuffer;                    // Буфер вершин объекта, используемый для хранения вершин геометрии.
    RenderBuffer ObjectIndexBuffer;                     // Буфер индекса объекта, используемый для хранения индексов геометрии.
    DArray<VulkanCommandBuffer> GraphicsCommandBuffers; // Буферы графических команд, по одному на кадр.
    DArray<VkSemaphore> ImageAvailableSemaphores;       // Семафоры, используемые для обозначения доступности изображения, по одному на кадр.
    DArray<VkSemaphore> QueueCompleteSemaphores;        // Семафоры, используемые для обозначения доступности очереди, по одному на кадр.
    u32 InFlightFenceCount{};                           // Текущее количество бортовых ограждений.
    VkFence InFlightFences[2]{};                        // Бортовые ограждения, используемые для указания приложению, когда кадр занят/готов.
    VkFence ImagesInFlight[3]{};                        // Содержит указатели на заборы, которые существуют и находятся в собственности в другом месте, по одному на кадр.
    u32 ImageIndex{};                                   // Индекс текущего изображения.
    u32 CurrentFrame{};                                 // Текущий кадр.
    bool RecreatingSwapchain{false};                    // Указывает, воссоздается ли в данный момент цепочка обмена.
    bool RenderFlagChanged{false};
    VulkanGeometry geometries[VULKAN_MAX_GEOMETRY_COUNT]{};   // ЗАДАЧА: динамическим, копии геометрий хранятся в системе геометрий, возможно стоит хранить здесь указатели на геометрии
    RenderTarget WorldRenderTargets[3]{};               // Цели рендера, используемые для рендеринга мира, по одному на кадр.
    bool MultithreadingEnabled;                         // Указывает, поддерживает ли данное устройство многопоточность.

public:
    /// @brief Инициализирует рендер.
    /// @param window указатель на общий интерфейс рендера.
    /// @param config указатель на конфигурацию, которая будет использоваться при инициализации рендераа.
    /// @param OutWindowRenderTargetCount Указатель для хранения необходимого количества целей рендеринга для проходов рендеринга, нацеленных на окно.
    VulkanAPI();
    ~VulkanAPI();
    
    bool Initialize(const RenderingConfig& config, u8& OutWindowRenderTargetCount)        override;
    void ShutDown()                                                                       override;
    void Resized(u16 width, u16 height)                                                   override;
    bool BeginFrame(const FrameData& rFrameData)                                          override;
    bool EndFrame(const FrameData& rFrameData)                                            override;
    void ViewportSet(const FVec4& rect)                                                   override;
    void ViewportReset()                                                                  override;
    void ScissorSet(const FVec4& rect)                                                    override;
    void ScissorReset()                                                                   override;
    bool RenderpassBegin(Renderpass* pass, RenderTarget& target)                          override;
    bool RenderpassEnd(Renderpass* pass)                                                  override;

    void Load(const u8* pixels, Texture* texture)                                         override;
    void LoadTextureWriteable  (Texture* texture)                                         override;
    void TextureResize         (Texture* texture, u32 NewWidth, u32 NewHeight)            override;
    void TextureWriteData      (Texture* texture, u32 offset, u32 size, const u8* pixels) override;
    void TextureReadData       (Texture* texture, u32 offset, u32 size, void** OutMemory) override;
    void TextureReadPixel      (Texture* texture, u32 x, u32 y, u8** OutRgba)             override;
    void* TextureCopyData(const Texture* texture)                                         override;
    void Unload                (Texture* texture)                                         override;

        //////////////////////////////////////////////////////////////////////////////////////////////
                //                           Geometry                           //
    //////////////////////////////////////////////////////////////////////////////////////////////    
    
    bool CreateGeometry      (Geometry* geometry)                                                                   override;
    bool Load                (Geometry* geometry, u32 VertexOffset, u32 VertexSize, u32 IndexOffset, u32 IndexSize) override;
    void Unload              (Geometry* geometry)                                                                   override;
    void GeometryVertexUpdate(Geometry* geometry, u32 offset, u32 VertexCount, void* vertices)                      override;
    void DrawGeometry(const GeometryRenderData& data)                                                               override;

    // Методы относящиеся к шейдерам---------------------------------------------------------------------------------------------------------------------------------------------

    bool Load                          (Shader *shader, const ShaderConfig& config, Renderpass* renderpass, const DArray<Shader::Stage>& stages, const DArray<MString>& StageFilenames) override;
    void Unload                        (Shader* shader)                                                                                                                                   override;
    bool ShaderInitialize              (Shader* shader)                                                                                                                                   override;
    bool ShaderUse                     (Shader* shader)                                                                                                                                   override;
    bool ShaderApplyGlobals            (Shader* shader, bool NeedsUpdate)                                                                                                                 override;
    bool ShaderApplyInstance           (Shader* shader, bool NeedsUpdate)                                                                                                                 override;
    bool ShaderBindInstance            (Shader* shader, u32 InstanceID)                                                                                                                   override;
    bool ShaderAcquireInstanceResources(Shader* shader, u32 TextureMapCount, TextureMap** maps, u32& OutInstanceID)                                                                       override;
    bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID)                                                                                                                   override;
    bool SetUniform                    (Shader* shader, struct Shader::Uniform* uniform, const void* value)                                                                               override;

    bool TextureMapAcquireResources(TextureMap* map) override;
    void TextureMapReleaseResources(TextureMap* map) override;
    void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, RenderTarget& OutTarget) override;
    void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory = false) override;
    bool RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass, bool copy) override;
    void RenderpassDestroy(Renderpass* renderpass) override;

    Texture* WindowAttachmentGet(u8 index)  override;
    Texture* DepthAttachmentGet(u8 index)   override;
    u8 WindowAttachmentIndexGet()           override;
    u8 WindowAttachmentCountGet()           override;

        //////////////////////////////////////////////////////////////////////////////////////////////
                //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////////////////////////////

    bool  RenderBufferCreate         (const char* name, RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer& buffer) override;
    bool  RenderBufferCreateInternal (RenderBuffer& buffer)                                                                           override;
    void  RenderBufferDestroyInternal(RenderBuffer& buffer)                                                                           override;
    bool  RenderBufferBind           (RenderBuffer& buffer, u64 offset)                                                               override;
    bool  RenderBufferUnbind         (RenderBuffer& buffer)                                                                           override;
    void* RenderBufferMapMemory      (RenderBuffer& buffer, u64 offset, u64 size)                                                     override;
    void  RenderBufferUnmapMemory    (RenderBuffer& buffer, u64 offset, u64 size)                                                     override;
    bool  RenderBufferFlush          (RenderBuffer& buffer, u64 offset, u64 size)                                                     override;
    bool  RenderBufferRead           (RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory)                                   override;
    bool  RenderBufferResize         (RenderBuffer& buffer, u64 NewTotalSize)                                                         override;
    bool  RenderBufferLoadRange      (RenderBuffer& buffer, u64 offset, u64 size, const void* data)                                   override;
    bool  RenderBufferCopyRange      (RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size)           override;
    bool  RenderBufferDraw           (RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly)                              override;

    /// @brief Функция поиска индекса памяти заданного типа и с заданными свойствами.
    /// @param TypeFilter типы памяти для поиска.
    /// @param PropertyFlags обязательные свойства, которые должны присутствовать.
    /// @return Индекс найденного типа памяти. Возвращает -1, если не найден.
    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);
    /// @brief Указывает, поддерживает ли рендерер многопоточность.
    const bool& IsMultithreaded() override;
    bool FlagEnabled   (RendererConfigFlags flag)               override;
    void FlagSetEnabled(RendererConfigFlags flag, bool enabled) override;

    PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT;

private:
    void CreateCommandBuffers();
    bool RecreateSwapchain();
    bool CreateModule(VulkanShader* shader, const VulkanShaderStageConfig& config, VulkanShaderStage* ShaderStage);

    bool CreateVulkanAllocator(VkAllocationCallbacks* callbacks);

    bool VulkanBufferCopyRangeInternal(VkBuffer source, u64 SourceOffset, VkBuffer dest, u64 DestOffset, u64 size);
};
