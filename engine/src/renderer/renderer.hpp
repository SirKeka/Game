/// @file renderer.hpp
/// @author 
/// @brief Интерфейс рендеринга — единственное, что видит остальная часть движка. 
/// Он отвечает за передачу любых данных в/из бэкэнда рендеринга независимым способом.
/// @version 1.0
/// @date 
///
/// @copyright 
#pragma once

#include "defines.hpp"

#include "renderer_types.hpp"
#include "math/vertex.hpp"
#include "core/event.hpp"

class MWindow;
class LinearAllocator;
struct StaticMeshData;
struct PlatformState;
class VulkanAPI;
class Shader;

class Renderer
{
    u8 WindowRenderTargetCount{};   // Количество целей рендеринга. Обычно совпадает с количеством изображений swapchain.
    u32 FramebufferWidth{};         // Текущая ширина буфера кадра окна.
    u32 FramebufferHeight{};        // Текущая высота буфера кадра окна.
    bool resizing{};                // Указывает, изменяется ли размер окна в данный момент.
    u8 FramesSinceResize{};         // Текущее количество кадров с момента последней операции изменения размера. Устанавливается только если resizing = true. В противном случае 0.

    static RendererType* ptrRenderer;
public:
    /// @brief Инициализирует интерфейс/систему рендеринга.
    /// @param window указатель на класс окна/поверхности на которой орисовщик будет рисовать
    /// @param ApplicationName Имя приложения.
    /// @param type тип отрисовщика с которым первоначально будет инициализироваться интерфейс/система
    Renderer(MWindow* window, const char *ApplicationName, ERendererType type, LinearAllocator& SystemAllocator);
    ~Renderer();

    /// @brief Инициализирует интерфейс/систему рендеринга.
    /// @param window указатель на класс окна/поверхности на которой орисовщик будет рисовать
    /// @param ApplicationName Имя приложения.
    /// @param type тип отрисовщика с которым первоначально будет инициализироваться интерфейс/система
    /// @return true в случае успеха; в противном случае false.
    // bool Initialize(MWindow* window, const char *ApplicationName, ERendererType type);
    /// @brief Выключает систему рендеринга/интерфейс.
    void Shutdown();
    /// @brief Обрабатывает события изменения размера.
    /// @param width Новая ширина окна.
    /// @param height Новая высота окна.
    void OnResized(u16 width, u16 height);
    /// @brief Рисует следующий кадр, используя данные, предоставленные в пакете рендеринга.
    /// @param packet Указатель на пакет рендеринга, который содержит данные о том, что должно быть визуализировано.
    /// @return true в случае успеха; в противном случае false.
    bool DrawFrame(const RenderPacket& packet);

    /// @brief Функция/метод предоставляющая доступ к самому отрисовщику.
    /// @return указатель на отрисовщик вулкан.
    static MINLINE VulkanAPI* GetRenderer() { return reinterpret_cast<VulkanAPI*>(ptrRenderer); }
 
    /// @brief Загружает данные текстуры в графический процессор.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @param pixels Необработанные данные изображения, которые будут загружены в графический процессор.
    /// @return true в случае успеха, иначе false.
    static void Load(const u8* pixels, Texture* texture);
    /// @brief Загружает новую записываемую текстуру без записи в нее данных.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @return true в случае успеха, иначе false.
    static void LoadTextureWriteable(Texture* texture);
    /// @brief Изменяет размер текстуры. На этом уровне нет проверки возможности записи текстуры. 
    /// Внутренние ресурсы уничтожаются и воссоздаются при новом разрешении. Данные потеряны, и их необходимо перезагрузить.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param NewWidth новая ширина в пикселях.
    /// @param NewHeight новая высота в пикселях.
    static void TextureResize(Texture* texture, u32 NewWidth, u32 NewHeight);
    /// @brief Записывает данные в предоставленную текстуру. ПРИМЕЧАНИЕ: На этом уровне это может быть как записываемая, 
    /// так и незаписываемая текстура, поскольку она также обрабатывает начальную загрузку текстуры. Сама система текстур 
    /// должна отвечать за блокировку запросов на запись в недоступные для записи текстуры.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param pixels необработанные данные изображения, которые необходимо записать.
    static void TextureWriteData(Texture* texture, u32 offset, u32 size, const u8* pixels);

    /// @brief Считывает указанные данные из предоставленной текстуры.
    /// @param texture Указатель на текстуру для чтения.
    /// @param offset Смещение в байтах от начала данных для чтения.
    /// @param size Количество байтов для чтения.
    /// @param OutMemory Указатель на блок памяти для записи считанных данных.
    static void TextureReadData(Texture* texture, u32 offset, u32 size, void** OutMemory);

    /// @brief Считывает пиксель из предоставленной текстуры с указанной координатой x/y.
    /// @param texture Указатель на текстуру для чтения.
    /// @param x Координата x пикселя.
    /// @param y Координата y пикселя.
    /// @param OutRgba Указатель на массив u8 для хранения данных пикселей (должен быть sizeof(u8) * 4)
    static void TextureReadPixel(Texture* texture, u32 x, u32 y, u8** OutRgba);

    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    static void Unload(Texture* texture);

    /// @brief Загружает данные геометрии в графический процессор.------------------------------------------------------------------------------------
    /// @param gid указатель на геометрию, которую требуется загрузить.
    /// @param VertexSize размер каждой вершины.
    /// @param VertexCount количество вершин.
    /// @param vertices массив вершин.
    /// @param IndexSize размер каждого индекса.
    /// @param IndexCount количество индексов.
    /// @param indices индексный массив.
    /// @return true в случае успеха; в противном случае false.
    static bool Load(GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices);
    /// @brief Уничтожает заданную геометрию, освобождая ресурсы графического процессора.-------------------------------------------------------------
    /// @param gid указатель на геометрию, которую нужно уничтожить.
    static void Unload(GeometryID* gid);
    /// @brief Рисует заданную геометрию. Должен вызываться только внутри прохода рендеринга, внутри кадра.
    /// @param data Данные рендеринга геометрии, которая должна быть нарисована.
    static void DrawGeometry(const GeometryRenderData& data);

    /// @brief Устанавливает область просмотра рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник области просмотра, который необходимо установить.
    static void ViewportSet(const FVec4& rect);

    /// @brief Сбрасывает область просмотра на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    static void ViewportReset();

    /// @brief Устанавливает ножницы рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник ножниц, который необходимо установить.
    static void ScissorSet(const FVec4& rect);

    /// @brief Сбрасывает ножницы на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    static void ScissorReset();

    /// @brief Начинает проход рендеринга с указанной целью.
    /// @param pass указатель на проход рендеринга для начала.
    /// @param target указатель на цель рендеринга для использования.
    /// @return тrue в случае успеха иначе false.
    static bool RenderpassBegin(Renderpass* pass, RenderTarget& target);

    /// @brief Завершает проход рендеринга с указанным идентификатором.
    /// @param pass указатель на проход рендеринга для завершения.
    /// @return тrue в случае успеха иначе false.
    static bool RenderpassEnd(Renderpass* pass);

    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.---------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param RenderpassID идентификатор прохода рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    static bool Load(Shader* shader, const Shader::Config& config, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const Shader::Stage* stages);
    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    static void Unload(Shader* shader);

    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    static bool ShaderInitialize(Shader* shader);

    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    static bool ShaderUse(Shader* shader);

    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    static bool ShaderApplyGlobals(Shader* shader);

    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    static bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate);

    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param maps массив указателей текстурных карт. Должен быть один на текстуру в экземпляре.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    static bool ShaderAcquireInstanceResources(Shader* shader, TextureMap** maps, u32& OutInstanceID);

    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    static bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID);

    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    static bool SetUniform(Shader* shader, struct Shader::Uniform* uniform, const void* value);

    /// @brief Получает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстуры, для которой нужно получить ресурсы.
    /// @return true в случае успеха; в противном случае false.
    static bool TextureMapAcquireResources(TextureMap* map);

    /// @brief Освобождает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстур, из которой необходимо освободить ресурсы.
    static void TextureMapReleaseResources(TextureMap* map);

    /// @brief Создает новую цель рендеринга, используя предоставленные данные.
    /// @param AttachmentCount количество вложений (указателей текстур).
    /// @param attachments массив вложений (указателей текстур).
    /// @param pass указатель на проход рендеринга, с которым связана цель рендеринга.
    /// @param width ширина цели рендеринга в пикселях.
    /// @param height высота цели рендеринга в пикселях.
    /// @param OutTarget указатель для хранения новой созданной цели рендеринга.
    static void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, RenderTarget& OutTarget);
    
    /// @brief Уничтожает предоставленную цель рендеринга.
    /// @param target указатель на цель рендеринга, которую нужно уничтожить.
    /// @param FreeInternalMemory указывает, следует ли освободить внутреннюю память.
    static void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory);

    /// @brief Создает новый проход рендеринга.
    /// @param config Константная ссылка на конфигурацию, которая будет использоваться при создании прохода рендеринга.
    /// @param OutRenderpass указатель на общий проход рендеринга.
    static bool RenderpassCreate(const RenderpassConfig& config, Renderpass& OutRenderpass);
    
    /// @brief Уничтожает указанный renderpass.
    /// @param OutRenderpass указатель на renderpass, который необходимо уничтожить.
    static void RenderpassDestroy(Renderpass* OutRenderpass);

    /// @brief Пытается получить цель визуализации окна по указанному индексу.
    /// @param index индекс вложения для получения. Должен быть в пределах диапазона количества целей визуализации окна.
    /// @return указатель на вложение текстуры в случае успеха; в противном случае 0.
    static Texture* WindowAttachmentGet(u8 index);

    /// @return Возвращает указатель на основную цель текстуры глубины.
    static Texture* DepthAttachmentGet(u8 index);

    /// @return Возвращает текущий индекс прикрепления окна.
    static u8 WindowAttachmentIndexGet();

    /// @brief Возвращает количество вложений, необходимых для целей визуализации на основе окна.
    MAPI static u8 WindowAttachmentCountGet();
    
    /// @brief Указывает, поддерживает ли рендерер многопоточность.
    static const bool& IsMultithreaded();

    /// @brief Указывает, включен ли предоставленный флаг рендерера. Если передано несколько флагов, все они должны быть установлены, чтобы вернуть значение true.
    /// @param flag проверяемый флаг.
    /// @return True, если флаг(и) установлены; в противном случае false.
    MAPI static bool FlagEnabled(RendererConfigFlags flag);

    /// @brief Устанавливает, включены ли включенные флаг(и). Если передано несколько флагов, несколько из них устанавливаются одновременно.
    /// @param flag проверяемый флаг.
    /// @param enabled Указывает, следует ли включать флаг(и).
    MAPI static void FlagSetEnabled(RendererConfigFlags flag, bool enabled);

    //////////////////////////////////////////////////////////////////////
    //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////

    /// @brief Создает и назначает буфер, специфичный для бэкэнда рендерера.
    ///
    /// @param buffer Указатель для создания внутреннего буфера.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferCreate(RenderBuffer& buffer);
    static bool RenderBufferCreate(RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer& buffer);

    /// @brief Уничтожает указанный буфер.
    ///
    /// @param buffer Указатель на буфер, который необходимо уничтожить.
    static void RenderBufferDestroy(RenderBuffer& buffer);

    /// @brief Связывает заданный буфер с указанным смещением.
    ///
    /// @param buffer Указатель на буфер для привязки.
    /// @param offset Смещение в байтах от начала буфера.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferBind(RenderBuffer& buffer, u64 offset);
    /// @brief Отменяет привязку указанного буфера.
    ///
    /// @param buffer Указатель на буфер, который необходимо отменить.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferUnbind(RenderBuffer& buffer);

    /// @brief Отображает память из указанного буфера в предоставленном диапазоне в блок памяти и возвращает его. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отображения.
    /// @param offset Количество байтов от начала буфера для отображения.
    /// @param size Объем памяти в буфере для отображения.
    /// @returns Отображенный блок памяти. Освобожден и недействителен после отмены отображения.
    static void* RenderBufferMapMemory(RenderBuffer& buffer, u64 offset, u64 size);
    /// @brief Отменяет отображение памяти из указанного буфера в предоставленном диапазоне в блок памяти. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для отмены отображения.
    /// @param size Объем памяти в буфере для отмены отображения.
    static void RenderBufferUnmapMemory(RenderBuffer& buffer, u64 offset, u64 size);

    /// @brief Очищает буферную память в указанном диапазоне. Должно быть сделано после записи.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для очистки.
    /// @param size Объем памяти в буфере для очистки.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferFlush(RenderBuffer& buffer, u64 offset, u64 size);

    /// @brief Считывает память из предоставленного буфера в указанном диапазоне в выходную переменную.
    /// @param buffer указатель на буфер для чтения.
    /// @param offset количество байтов от начала буфера для чтения.
    /// @param size объем памяти в буфере для чтения.
    /// @param OutMemory указатель на блок памяти для чтения. Должен быть соответствующего размера.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferRead(RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory);

    /// @brief Изменяет размер указанного буфера на NewTotalSize. NewTotalSize должен быть больше текущего размера буфера. Данные из старого внутреннего буфера копируются.
    ///
    /// @param buffer указатель на буфер, размер которого необходимо изменить.
    /// @param NewTotalSize новый размер в байтах. Должен быть больше текущего размера.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferResize(RenderBuffer& buffer, u64 NewTotalSize);

    /// @brief Загружает предоставленные данные в указанный диапазон указанного буфера.
    ///
    /// @param buffer указатель на буфер, в который нужно загрузить данные.
    /// @param offset смещение в байтах от начала буфера.
    /// @param size размер данных в байтах для загрузки.
    /// @param data данные для загрузки.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferLoadRange(RenderBuffer& buffer, u64 offset, u64 size, const void* data);

    /// @brief Копирует данные в указанном диапазоне из исходного буфера в целевой буфер.
    ///
    /// @param source указатель на исходный буфер, из которого копируются данные.
    /// @param SourceOffset смещение в байтах от начала исходного буфера.
    /// @param dest указатель на целевой буфер, в который копируются данные.
    /// @param DestOffset смещение в байтах от начала целевого буфера.
    /// @param size размер копируемых данных в байтах.
    /// @returns True в случае успеха; в противном случае false.
    static bool RenderBufferCopyRange(RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size);

    /// @brief Пытается нарисовать содержимое предоставленного буфера с указанным смещением и количеством элементов. 
    /// Предназначено только для использования с буферами вершин и индексов.
    ///
    /// @param buffer указатель на буфер для рисования.
    /// @param offset смещение в байтах от начала буфера.
    /// @param ElementCount количество элементов для рисования.
    /// @param BindOnly только связывает буфер, но не вызывает draw.
    /// @return True в случае успеха; в противном случае false.
    static bool RenderBufferDraw(RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly);
};

