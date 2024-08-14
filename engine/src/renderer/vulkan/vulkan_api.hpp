#pragma once

#include "renderer/renderer_types.hpp"

#include "core/asserts.hpp"
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_shader.hpp"
#include "resources/geometry.hpp"
#include "math/vertex.hpp"

// Проверяет возвращаемое значение данного выражения на соответствие VK_SUCCESS.
#define VK_CHECK(expr)           \
{                                \
    MASSERT(expr == VK_SUCCESS); \
}

constexpr u32 VULKAN_MAX_REGISTERED_RENDERPASSES = 31;

class VulkanAPI : public RendererType
{
public:
    f32 FrameDeltaTime{};                               // Время в секундах с момента последнего кадра.
    u32 FramebufferWidth{};                             // Текущая ширина фреймбуфера.
    u32 FramebufferHeight{};                            // Текущая высота фреймбуфера.
    u64 FramebufferSizeGeneration{};                    // Текущее поколение размера кадрового буфера. Если он не соответствует FramebufferSizeLastGeneration, необходимо создать новый.
    u64 FramebufferSizeLastGeneration{};                // Генерация кадрового буфера при его последнем создании. При обновлении установите значение FramebufferSizeGeneration.
    VkInstance instance{};                              // Дескриптор внутреннего экземпляра Vulkan.
    VkAllocationCallbacks* allocator{nullptr};          // Внутренний распределитель Vulkan.
    VkSurfaceKHR surface{};                             // Внутренняя поверхность Vulkan, на которой будет отображаться окно.

#if defined(_DEBUG)
    VkDebugUtilsMessengerEXT DebugMessenger;            // Мессенджер отладки, если он активен.
#endif

    VulkanDevice Device{};                              // Устройство Vulkan.
    VulkanSwapchain swapchain{};                        // Цепочка подкачки.
    void* RenderpassTableBlock{nullptr};                // Блок памяти для хештаблицы подпроходов рендера.
    HashTable<u32> RenderpassTable;                     // Хештаблица подпроходов рендера.
    Renderpass RegisteredPasses[VULKAN_MAX_REGISTERED_RENDERPASSES]; // Зарегистрированные рендер-проходы
    VulkanBuffer ObjectVertexBuffer{};                  // Буфер вершин объекта, используемый для хранения вершин геометрии.
    VulkanBuffer ObjectIndexBuffer{};                   // Буфер индекса объекта, используемый для хранения индексов геометрии.
    DArray<VulkanCommandBuffer> GraphicsCommandBuffers; // Буферы графических команд, по одному на кадр.
    DArray<VkSemaphore> ImageAvailableSemaphores;       // Семафоры, используемые для обозначения доступности изображения, по одному на кадр.
    DArray<VkSemaphore> QueueCompleteSemaphores;        // Семафоры, используемые для обозначения доступности очереди, по одному на кадр.
    u32 InFlightFenceCount{};                           // Текущее количество бортовых ограждений.
    VkFence InFlightFences[2]{};                        // Бортовые ограждения, используемые для указания приложению, когда кадр занят/готов.
    VkFence ImagesInFlight[3]{};                        // Содержит указатели на заборы, которые существуют и находятся в собственности в другом месте, по одному на кадр.
    u32 ImageIndex{};                                   // Индекс текущего изображения.
    u32 CurrentFrame{};                                 // Текущий кадр.
    bool RecreatingSwapchain{false};                    // Указывает, воссоздается ли в данный момент цепочка обмена.
    Geometry geometries[VULKAN_MAX_GEOMETRY_COUNT]{};   // ЗАДАЧА: динамическим, копии геометрий хранятся в системе геометрий, возможно стоит хранить здесь указатели на геометрии
    RenderTarget WorldRenderTargets[3]{};               // Цели рендера, используемые для рендеринга мира, по одному на кадр.

public:
    /// @brief Инициализирует рендер.
    /// @param window указатель на общий интерфейс рендера.
    /// @param config указатель на конфигурацию, которая будет использоваться при инициализации рендераа.
    /// @param OutWindowRenderTargetCount Указатель для хранения необходимого количества целей рендеринга для проходов рендеринга, нацеленных на окно.
    VulkanAPI(class MWindow* window, const RendererConfig& config, u8& OutWindowRenderTargetCount);
    ~VulkanAPI();
    
    void ShutDown() override;
    void Resized(u16 width, u16 height) override;
    bool BeginFrame(f32 Deltatime) override;
    bool EndFrame(f32 DeltaTime) override;
    bool RenderpassBegin(Renderpass* pass, RenderTarget& target) override;
    bool RenderpassEnd(Renderpass* pass) override;
    Renderpass* GetRenderpass(const MString& name) override;

    /// @brief Загружает данные текстуры в графический процессор.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @param pixels Необработанные данные изображения, которые будут загружены в графический процессор.
    /// @return true в случае успеха, иначе false.
    void Load(const u8* pixels, Texture* texture) override;
    /// @brief Загружает новую записываемую текстуру без записи в нее данных.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @return true в случае успеха, иначе false.
    void LoadTextureWriteable(Texture* texture) override;
    /// @brief Изменяет размер текстуры. На этом уровне нет проверки возможности записи текстуры. 
    /// Внутренние ресурсы уничтожаются и воссоздаются при новом разрешении. Данные потеряны, и их необходимо перезагрузить.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param NewWidth новая ширина в пикселях.
    /// @param NewHeight новая высота в пикселях.
    void TextureResize(Texture* texture, u32 NewWidth, u32 NewHeight) override;
    /// @brief Записывает данные в предоставленную текстуру. ПРИМЕЧАНИЕ: На этом уровне это может быть как записываемая, 
    /// так и незаписываемая текстура, поскольку она также обрабатывает начальную загрузку текстуры. Сама система текстур 
    /// должна отвечать за блокировку запросов на запись в недоступные для записи текстуры.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param pixels необработанные данные изображения, которые необходимо записать.
    void TextureWriteData(Texture* texture, u32 offset, u32 size, const u8* pixels) override;
    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    void Unload(Texture* texture) override;
    bool Load(GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices) override;
    void Unload(GeometryID* gid) override;
    void DrawGeometry(const GeometryRenderData& data) override;

    // Методы относящиеся к шейдерам---------------------------------------------------------------------------------------------------------------------------------------------

    bool Load(Shader* shader, const ShaderConfig& config, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage* stages) override;
    void Unload(Shader* shader) override;
    bool ShaderInitialize(Shader* shader) override;
    bool ShaderUse(Shader* shader) override;
    bool ShaderApplyGlobals(Shader* shader) override;
    bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate) override;
    bool ShaderAcquireInstanceResources(Shader* shader, TextureMap** maps, u32& OutInstanceID) override;
    bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID) override;
    bool SetUniform(Shader* shader, struct ShaderUniform* uniform, const void* value) override;
    bool TextureMapAcquireResources(TextureMap& map) override;
    void TextureMapReleaseResources(TextureMap* map) override;
    void RenderTargetCreate(u8 AttachmentCount, Texture** attachments, Renderpass* pass, u32 width, u32 height, RenderTarget& OutTarget) override;
    void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory = false) override;
    void RenderpassCreate(Renderpass* OutRenderpass, f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass) override;
    void RenderpassDestroy(Renderpass* OutRenderpass) override;
    Texture* WindowAttachmentGet(u8 index) override;
    Texture* DepthAttachmentGet() override;
    u8 WindowAttachmentIndexGet() override;

    void* operator new(u64 size);
    /// @brief Функция поиска индекса памяти заданного типа и с заданными свойствами.
    /// @param TypeFilter типы памяти для поиска.
    /// @param PropertyFlags обязательные свойства, которые должны присутствовать.
    /// @return Индекс найденного типа памяти. Возвращает -1, если не найден.
    i32 FindMemoryIndex(u32 TypeFilter, VkMemoryPropertyFlags PropertyFlags);

private:
    void CreateCommandBuffers();
    bool RecreateSwapchain();
    bool CreateBuffers();
    bool CreateModule(VulkanShader* shader, const VulkanShaderStageConfig& config, VulkanShaderStage* ShaderStage);

    bool UploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, VulkanBuffer& buffer, u64& OutOffset, u64 size, const void* data);
    void FreeDataRange(VulkanBuffer* buffer, u64 offset, u64 size);
    // Обратный вызов, который будет выполнен, когда бэкэнд потребует обновления/повторной генерации целей рендеринга.
    const RendererConfig::PFN_Method& OnRendertargetRefreshRequired;
    //void (Renderer::*OnRendertargetRefreshRequired)();   
};
