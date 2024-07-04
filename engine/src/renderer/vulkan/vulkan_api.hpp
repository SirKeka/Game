#pragma once

#include "renderer/renderer_types.hpp"

#include "core/asserts.hpp"
#include "vulkan_image.hpp"
#include "vulkan_device.hpp"
#include "vulkan_renderpass.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_shader.hpp"
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
    f32 FrameDeltaTime{0};                              // Время в секундах с момента последнего кадра.
    u32 FramebufferWidth{0};                            // Текущая ширина фреймбуфера.
    u32 FramebufferHeight{0};                           // Текущая высота фреймбуфера.
    u64 FramebufferSizeGeneration{0};                   // Текущее поколение размера кадрового буфера. Если он не соответствует FramebufferSizeLastGeneration, необходимо создать новый.
    u64 FramebufferSizeLastGeneration{0};               // Генерация кадрового буфера при его последнем создании. При обновлении установите значение FramebufferSizeGeneration.
    VkInstance instance{};                              // Дескриптор внутреннего экземпляра Vulkan.
    VkAllocationCallbacks* allocator{nullptr};          // Внутренний распределитель Vulkan.
    VkSurfaceKHR surface{};                             // Внутренняя поверхность Vulkan, на которой будет отображаться окно.

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;            // Мессенджер отладки, если он активен.
#endif

    VulkanDevice Device{};                              // Устройство Vulkan.
    VulkanSwapchain swapchain{};                        // Цепочка подкачки
    VulkanRenderpass MainRenderpass{};                  // Основной проход рендеринга мира.
    VulkanRenderpass UI_Renderpass{};                   // Проход рендеринга пользовательского интерфейса.
    VulkanBuffer ObjectVertexBuffer{};                  // Буфер вершин объекта, используемый для хранения вершин геометрии.
    VulkanBuffer ObjectIndexBuffer{};                   // Буфер индекса объекта, используемый для хранения индексов геометрии.
    DArray<VulkanCommandBuffer> GraphicsCommandBuffers; // Буферы графических команд, по одному на кадр.
    DArray<VkSemaphore> ImageAvailableSemaphores;       // Семафоры, используемые для обозначения доступности изображения, по одному на кадр.
    DArray<VkSemaphore> QueueCompleteSemaphores;        // Семафоры, используемые для обозначения доступности очереди, по одному на кадр.
    u32 InFlightFenceCount{};                           // Текущее количество бортовых ограждений.
    VkFence InFlightFences[2]{};                        // Бортовые ограждения, используемые для указания приложению, когда кадр занят/готов.
    VkFence ImagesInFlight[3]{};                        // Содержит указатели на заборы, которые существуют и находятся в собственности в другом месте, по одному на кадр.
    u32 ImageIndex{0};                                  // Индекс текущего изображения.
    u32 CurrentFrame{0};                                // Текущий кадр.
    bool RecreatingSwapchain{false};                    // Указывает, воссоздается ли в данный момент цепочка обмена.
    Geometry geometries[VULKAN_MAX_GEOMETRY_COUNT]{};   // СДЕЛАТЬ: динамическим, копии геометрий хранятся в системе геометрий, возможно стоит хранить здесь указатели на геометрии
    VkFramebuffer WorldFramebuffers[3]{};               // Буферы кадров, используемые для рендеринга мира, по одному на кадр.

private:
    u32 CachedFramebufferWidth{};
    u32 CachedFramebufferHeight{};

public:
    VulkanAPI(class MWindow* window, const char* ApplicationName);
    ~VulkanAPI();
    
    // bool Initialize(MWindow* window, const char* ApplicationName) override;
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    bool EndFrame(f32 DeltaTime) override;
    bool BeginRenderpass(u8 RenderpassID) override;
    bool EndRenderpass(u8 RenderpassID) override;

    // TODO: перенести в класс системы визуализации
    bool Load(GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices) override;
    void Unload(GeometryID* gid) override;
    void DrawGeometry(const GeometryRenderData& data) override;

    // Методы относящиеся к шейдерам---------------------------------------------------------------------------------------------------------------------------------------------
    
    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    /// @param shader указатель на шейдер.
    /// @param RenderpassID идентификатор прохода рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    bool Load(Shader* shader, u8 RenderpassID, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage* stages) override;
    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    void Unload(Shader* shader) override;
    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    bool ShaderInitialize(Shader* shader) override;
    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    bool ShaderUse(Shader* shader) override;
    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    bool ShaderApplyGlobals(Shader* shader) override;
    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate) override;
    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    bool ShaderAcquireInstanceResources(Shader* shader, u32& OutInstanceID) override;
    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID) override;
    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    bool SetUniform(Shader* shader, struct ShaderUniform* uniform, const void* value) override;

    void* operator new(u64 size);
    /// @brief Функция поиска индекса памяти заданного типа и с заданными свойствами.
    /// @param TypeFilter типы памяти для поиска.
    /// @param PropertyFlags обязательные свойства, которые должны присутствовать.
    /// @return Индекс найденного типа памяти. Возвращает -1, если не найден.
    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);

private:
    void CreateCommandBuffers();
    void RegenerateFramebuffers();
    bool RecreateSwapchain();
    bool CreateBuffers();
    bool CreateModule(VulkanShader* shader, const VulkanShaderStageConfig& config, VulkanShaderStage* ShaderStage);

    bool UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer& buffer, u64& OutOffset, u64 size, const void* data);
    void FreeDataRange(VulkanBuffer* buffer, u64 offset, u64 size);
    
};
