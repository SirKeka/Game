#pragma once

#include <core/asserts.h>

#include "renderer/renderer_plugin.h"

#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_command_buffer.h"
#include "vulkan_shader.h"
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
    Rect2D ViewportRect;                                // Прямоугольник области просмотра.
    Rect2D ScissorRect;                                 // Прямоугольник ножниц.
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

    VulkanDevice device;                                // Устройство Vulkan.
    VulkanSwapchain swapchain;                          // Цепочка подкачки.
    RenderBuffer staging;                               // Многоразовый промежуточный буфер для передачи данных из ресурса в буфер, предназначенный только для графического процессора.
    DArray<VulkanCommandBuffer> GraphicsCommandBuffers; // Буферы графических команд, по одному на кадр.
    DArray<VkSemaphore> ImageAvailableSemaphores;       // Семафоры, используемые для обозначения доступности изображения, по одному на кадр.
    DArray<VkSemaphore> QueueCompleteSemaphores;        // Семафоры, используемые для обозначения доступности очереди, по одному на кадр.
    DArray<VkSampler> samplers;                         // Коллекция семплеров
    u32 InFlightFenceCount{};                           // Текущее количество бортовых ограждений.
    VkFence InFlightFences[2]{};                        // Бортовые ограждения, используемые для указания приложению, когда кадр занят/готов.
    u32 ImageIndex{};                                   // Индекс текущего изображения.
    u32 CurrentFrame{};                                 // Текущий кадр.
    bool RecreatingSwapchain{false};                    // Указывает, воссоздается ли в данный момент цепочка обмена.
    bool RenderFlagChanged{false};
    bool ValidationEnabled;
    RenderTarget WorldRenderTargets[3]{};               // Цели рендера, используемые для рендеринга мира, по одному на кадр.
    bool MultithreadingEnabled;                         // Указывает, поддерживает ли данное устройство многопоточность.
    struct Shader* BoundShader;                         // Указатель на текущий привязанный шейдер.

    struct shaderc_compiler* ShaderCompiler;           // Используется для динамической компиляции шейдеров Vulkan (с использованием библиотеки shaderc).

public:
    /// @brief Инициализирует рендер.
    /// @param window указатель на общий интерфейс рендера.
    /// @param config указатель на конфигурацию, которая будет использоваться при инициализации рендераа.
    /// @param OutWindowRenderTargetCount Указатель для хранения необходимого количества целей рендеринга для проходов рендеринга, нацеленных на окно.
    VulkanAPI();
    ~VulkanAPI();
    
    bool Initialize(const RenderingConfig& config, u8& OutWindowRenderTargetCount)        override;
    void Shutdown()                                                                       override;
    void Resized(u16 width, u16 height)                                                   override;
    void BeginDebugLabel(const char* LabelText, FVec3 colour)                             override;
    void EndDebugLabel()                                                                  override;
    bool PrepareFrame(const FrameData& rFrameDat)                                         override;
    bool Begin(const FrameData& rFrameData)                                               override;
    bool End(const FrameData& rFrameData)                                                 override;
    bool Present(const FrameData& rFrameData)                                             override;
    void SetViewport(const Rect2D& rect)                                                  override;
    void ViewportReset()                                                                  override;
    void SetScissor(const Rect2D& rect)                                                   override;
    void ScissorReset()                                                                   override;
    void SetStencilTestEnabled(bool enabled)                                              override;
    void SetDepthTestEnabled(bool enabled)                                                override;
    void SetStencilReference(u32 reference)                                               override;
    void SetStencilOp(RendererStencilOp FailOp, RendererStencilOp PassOp, RendererStencilOp DepthFailOp, RendererCompareOp CompareOp) override;
    void SetStencilCompareMask(u32 CompareMask)                                           override;
    void SetStencilWriteMask(u32 WriteMask)                                               override;
    void SetWinding(RendererWinding winding)                                              override;
    bool RenderpassBegin(Renderpass* pass, RenderTarget& target)                          override;
    bool RenderpassEnd(Renderpass* pass)                                                  override;

    void Load(const u8* pixels, Texture* texture)                                                                      override;
    void LoadTextureWriteable  (Texture* texture)                                                                      override;
    void TextureResize         (Texture* texture, u32 NewWidth, u32 NewHeight)                                         override;
    void TextureWriteData      (Texture* texture, u32 offset, u32 size, const u8* pixels, bool IncludeInFrameWorkload) override;
    void TextureReadData       (Texture* texture, u32 offset, u32 size, void** OutMemory)                              override;
    void TextureReadPixel      (Texture* texture, u32 x, u32 y, u8** OutRgba)                                          override;
    void* TextureCopyData(const Texture* texture)                                                                      override;
    void Unload                (Texture* texture)                                                                      override;

        //////////////////////////////////////////////////////////////////////////////////////////////
                //                               Geometry                           //
    //////////////////////////////////////////////////////////////////////////////////////////////    
    
    bool CreateGeometry      (Geometry* geometry)                                              override;

    // Методы относящиеся к шейдерам---------------------------------------------------------------------------------------------------------------------------------------------

    bool ShaderCreate                  (Shader* shader, const ShaderConfig& config, Renderpass* renderpass)               override;
    void ShaderDestroy                 (Shader* shader)                                                                   override;
    bool ShaderInitialize              (Shader* shader)                                                                   override;
    bool ShaderReload                  (Shader* shader)                                                                   override;
    bool ShaderSupportsWireframe       (Shader* shader)                                                                   override;
    bool ShaderUse                     (Shader* shader)                                                                   override;
    bool ShaderApplyGlobals            (Shader* shader, bool NeedsUpdate, const FrameData& rFrameData)                    override;
    bool ShaderApplyInstance           (Shader* shader, bool NeedsUpdate, const FrameData& rFrameData)                    override;
    bool ShaderApplyLocal              (Shader* shader, const FrameData& rFrameData)                                      override;
    bool ShaderBindGlobals             (Shader* shader)                                                                   override;
    bool ShaderBindInstance            (Shader* shader, u32 InstanceID)                                                   override;
    bool ShaderBindLocal               (Shader* shader)                                                                   override;
    bool ShaderAcquireInstanceResources(Shader* shader, const ShaderInstanceResourceConfig& config, u32& OutInstanceID)   override;
    bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID)                                                   override;
    bool ShaderSetUniform              (Shader* shader, struct ShaderUniform* uniform, u32 ArrayIndex, const void* value) override;

    bool TextureMapResourceAcquire (TextureMap* map) override;
    void TextureMapDestroy(TextureMap* map) override;
    bool TextureMapRefresh(TextureMap* map) override;

    void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, u32 LayerIndex, RenderTarget& OutTarget) override;
    void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory = false) override;
    bool RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass, bool copy) override;
    void RenderpassDestroy(Renderpass* renderpass) override;

    Texture* GetWindowAttachment(u8 index)  override;
    Texture* GetDepthAttachment(u8 index)   override;
    u8 GetWindowAttachmentIndex()           override;
    u8 GetWindowAttachmentCount()           override;

        //////////////////////////////////////////////////////////////////////////////////////////////
                //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////////////////////////////

    bool  RenderBufferCreate     (RenderBuffer& buffer)                                                                                              override;
    void  RenderBufferDestroy    (RenderBuffer& buffer)                                                                                              override;
    bool  RenderBufferBind       (RenderBuffer& buffer, u64 offset)                                                                                  override;
    bool  RenderBufferUnbind     (RenderBuffer& buffer)                                                                                              override;
    void* RenderBufferMapMemory  (RenderBuffer& buffer, u64 offset, u64 size)                                                                        override;
    void  RenderBufferUnmapMemory(RenderBuffer& buffer, u64 offset, u64 size)                                                                        override;
    bool  RenderBufferFlush      (RenderBuffer& buffer, u64 offset, u64 size)                                                                        override;
    bool  RenderBufferRead       (RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory)                                                      override;
    bool  RenderBufferResize     (RenderBuffer& buffer, u64 NewTotalSize)                                                                            override;
    bool  RenderBufferLoadRange  (RenderBuffer& buffer, u64 offset, u64 size, const void* data, bool IncludeInFrameWorkload)                         override;
    bool  RenderBufferCopyRange  (RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size, bool IncludeInFrameWorkload) override;
    bool  RenderBufferDraw       (RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly)                                                 override;

    /// @brief Функция поиска индекса памяти заданного типа и с заданными свойствами.
    /// @param TypeFilter типы памяти для поиска.
    /// @param PropertyFlags обязательные свойства, которые должны присутствовать.
    /// @return Индекс найденного типа памяти. Возвращает -1, если не найден.
    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);
    /// @brief Указывает, поддерживает ли рендерер многопоточность.
    const bool& IsMultithreaded()                               override;
    bool FlagEnabled   (RendererConfigFlags flag)               override;
    void FlagSetEnabled(RendererConfigFlags flag, bool enabled) override;

    void WaitForIdle()                                          override;

    PFN_vkCmdSetPrimitiveTopologyEXT vkCmdSetPrimitiveTopologyEXT;
    PFN_vkCmdSetFrontFaceEXT vkCmdSetFrontFaceEXT;
    PFN_vkCmdSetStencilTestEnableEXT vkCmdSetStencilTestEnableEXT;
    PFN_vkCmdSetDepthTestEnableEXT vkCmdSetDepthTestEnableEXT;
    PFN_vkCmdSetStencilOpEXT vkCmdSetStencilOpEXT;

private:
    void CreateCommandBuffers();
    bool RecreateSwapchain();
    bool CreateSampler(TextureMap *map, VkSampler *sampler);
    bool CreateShaderModule(Shader* shader, const ShaderStageConfig& config, VulkanShaderStage& ShaderStage);
    bool ShaderCreateModulesAndPipelines(Shader* shader);

    bool CreateVulkanAllocator(VkAllocationCallbacks* callbacks);

    bool VulkanBufferCopyRangeInternal(VkBuffer source, u64 SourceOffset, VkBuffer dest, u64 DestOffset, u64 size, bool IncludeInFrameWorkload);
};
