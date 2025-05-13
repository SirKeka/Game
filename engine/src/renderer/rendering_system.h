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

#include "renderer_types.h"
#include "math/vertex.h"
#include "core/event.hpp"

class MWindow;
class LinearAllocator;
struct StaticMeshData;
struct PlatformState;
class VulkanAPI;
struct Shader;
struct FrameData;

struct RenderingSystemConfig
{
    const char *ApplicationName;
    RendererPlugin* plugin;
};

namespace RenderingSystem
{
    /// @brief Инициализирует интерфейс/систему рендеринга.
    /// @param window указатель на класс окна/поверхности на которой орисовщик будет рисовать
    /// @param ApplicationName Имя приложения.
    /// @param type тип отрисовщика с которым первоначально будет инициализироваться интерфейс/система
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);

    /// @brief Выключает систему рендеринга/интерфейс.
    void Shutdown();

    /// @brief Обрабатывает события изменения размера.
    /// @param width Новая ширина окна.
    /// @param height Новая высота окна.
    void OnResized(u16 width, u16 height);

    /// @brief Рисует следующий кадр, используя данные, предоставленные в пакете рендеринга.
    /// @param packet константная ссылка на пакет рендеринга, который содержит данные о том, что должно быть визуализировано.
    /// @param rFrameData константная ссылка на данные текущего кадра
    /// @return true в случае успеха; в противном случае false.
    bool DrawFrame(const RenderPacket& packet, const FrameData& rFrameData);

    /// @brief Функция/метод предоставляющая доступ к самому отрисовщику.
    /// @return указатель на отрисовщик вулкан.
    //MINLINE VulkanAPI* GetRenderer() { return reinterpret_cast<VulkanAPI*>(ptrRenderer); }
 
    /// @brief Загружает данные текстуры в графический процессор.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @param pixels Необработанные данные изображения, которые будут загружены в графический процессор.
    /// @return true в случае успеха, иначе false.
    void Load(const u8* pixels, Texture* texture);
    /// @brief Загружает новую записываемую текстуру без записи в нее данных.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @return true в случае успеха, иначе false.
    void LoadTextureWriteable(Texture* texture);
    /// @brief Изменяет размер текстуры. На этом уровне нет проверки возможности записи текстуры. 
    /// Внутренние ресурсы уничтожаются и воссоздаются при новом разрешении. Данные потеряны, и их необходимо перезагрузить.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param NewWidth новая ширина в пикселях.
    /// @param NewHeight новая высота в пикселях.
    void TextureResize(Texture* texture, u32 NewWidth, u32 NewHeight);
    /// @brief Записывает данные в предоставленную текстуру. ПРИМЕЧАНИЕ: На этом уровне это может быть как записываемая, 
    /// так и незаписываемая текстура, поскольку она также обрабатывает начальную загрузку текстуры. Сама система текстур 
    /// должна отвечать за блокировку запросов на запись в недоступные для записи текстуры.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param pixels необработанные данные изображения, которые необходимо записать.
    void TextureWriteData(Texture* texture, u32 offset, u32 size, const u8* pixels);

    /// @brief Считывает указанные данные из предоставленной текстуры.
    /// @param texture Указатель на текстуру для чтения.
    /// @param offset Смещение в байтах от начала данных для чтения.
    /// @param size Количество байтов для чтения.
    /// @param OutMemory Указатель на блок памяти для записи считанных данных.
    void TextureReadData(Texture* texture, u32 offset, u32 size, void** OutMemory);

    /// @brief Считывает пиксель из предоставленной текстуры с указанной координатой x/y.
    /// @param texture Указатель на текстуру для чтения.
    /// @param x Координата x пикселя.
    /// @param y Координата y пикселя.
    /// @param OutRgba Указатель на массив u8 для хранения данных пикселей (должен быть sizeof(u8) * 4)
    void TextureReadPixel(Texture* texture, u32 x, u32 y, u8** OutRgba);

    /// @brief Копирует данные структуры и возвращает указатель на них
    /// @param texture текстура данные которой нужно скопировать
    /// @return укахатель на данные
    void* TextureCopyData(const Texture* texture);

    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    void Unload(Texture* texture);

    /// @brief Загружает данные геометрии в графический процессор.------------------------------------------------------------------------------------
    /// @param gid указатель на геометрию, которую требуется загрузить.
    /// @param VertexSize размер каждой вершины.
    /// @param VertexCount количество вершин.
    /// @param vertices массив вершин.
    /// @param IndexSize размер каждого индекса.
    /// @param IndexCount количество индексов.
    /// @param indices индексный массив.
    /// @return true в случае успеха; в противном случае false.
    bool Load(GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize = 0, u32 IndexCount = 0, const void* indices = nullptr);
    
    /// @brief Обновляет данные вершин в заданной геометрии предоставленными данными в заданном диапазоне.
    /// @param g Указатель на геометрию, которую нужно создать.
    /// @param offset Смещение в байтах для обновления. 0, если обновление с начала.
    /// @param VertexCount Количество вершин, которые будут обновлены.
    /// @param vertices Данные вершин.
    void GeometryVertexUpdate(GeometryID* g, u32 offset, u32 VertexCount, void* vertices);

    /// @brief Уничтожает заданную геометрию, освобождая ресурсы графического процессора.-------------------------------------------------------------
    /// @param gid указатель на геометрию, которую нужно уничтожить.
    void Unload(GeometryID* gid);
    /// @brief Рисует заданную геометрию. Должен вызываться только внутри прохода рендеринга, внутри кадра.
    /// @param data Данные рендеринга геометрии, которая должна быть нарисована.
    void DrawGeometry(const GeometryRenderData& data);

    /// @brief Устанавливает область просмотра рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник области просмотра, который необходимо установить.
    void ViewportSet(const FVec4& rect);

    /// @brief Сбрасывает область просмотра на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    void ViewportReset();

    /// @brief Устанавливает ножницы рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник ножниц, который необходимо установить.
    void ScissorSet(const FVec4& rect);

    /// @brief Сбрасывает ножницы на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    void ScissorReset();

    /// @brief Начинает проход рендеринга с указанной целью.
    /// @param pass указатель на проход рендеринга для начала.
    /// @param target указатель на цель рендеринга для использования.
    /// @return тrue в случае успеха иначе false.
    bool RenderpassBegin(Renderpass* pass, RenderTarget& target);

    /// @brief Завершает проход рендеринга с указанным идентификатором.
    /// @param pass указатель на проход рендеринга для завершения.
    /// @return тrue в случае успеха иначе false.
    bool RenderpassEnd(Renderpass* pass);

    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.---------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param RenderpassID идентификатор прохода рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    bool Load(Shader *shader, const Shader::Config& config, Renderpass* renderpass, const DArray<Shader::Stage>& stages, const DArray<MString>& StageFilenames);
    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    void Unload(Shader* shader);

    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    bool ShaderInitialize(Shader* shader);

    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    bool ShaderUse(Shader* shader);

    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    bool ShaderApplyGlobals(Shader* shader);

    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate);

    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param TextureMapCount количество используемых текстурных карт.
    /// @param maps массив указателей текстурных карт. Должен быть один на текстуру в экземпляре.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    bool ShaderAcquireInstanceResources(Shader* shader, u32 TextureMapCount, TextureMap** maps, u32& OutInstanceID);

    /// @brief Связывает ресурсы экземпляра для использования и обновления.
    /// @param шейдер 
    /// @param InstanceID идентификатор экземпляра, который необходимо привязать.
    /// @return true в случае успеха, иначе false.
    bool ShaderBindInstance(Shader* shader, u32 InstanceID);

    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID);

    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    bool SetUniform(Shader* shader, struct Shader::Uniform* uniform, const void* value);

    /// @brief Получает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстуры, для которой нужно получить ресурсы.
    /// @return true в случае успеха; в противном случае false.
    bool TextureMapAcquireResources(TextureMap* map);

    /// @brief Освобождает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстур, из которой необходимо освободить ресурсы.
    void TextureMapReleaseResources(TextureMap* map);

    /// @brief Создает новую цель рендеринга, используя предоставленные данные.
    /// @param AttachmentCount количество вложений (указателей текстур).
    /// @param attachments массив вложений (указателей текстур).
    /// @param pass указатель на проход рендеринга, с которым связана цель рендеринга.
    /// @param width ширина цели рендеринга в пикселях.
    /// @param height высота цели рендеринга в пикселях.
    /// @param OutTarget указатель для хранения новой созданной цели рендеринга.
    void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, RenderTarget& OutTarget);
    
    /// @brief Уничтожает предоставленную цель рендеринга.
    /// @param target указатель на цель рендеринга, которую нужно уничтожить.
    /// @param FreeInternalMemory указывает, следует ли освободить внутреннюю память.
    void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory);

    /// @brief Создает новый проход рендеринга.
    /// @param config Константная ссылка на конфигурацию, которая будет использоваться при создании прохода рендеринга.
    /// @param OutRenderpass указатель на общий проход рендеринга.
    bool RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass);
    
    /// @brief Уничтожает указанный renderpass.
    /// @param OutRenderpass указатель на renderpass, который необходимо уничтожить.
    void RenderpassDestroy(Renderpass* OutRenderpass);

    /// @brief Пытается получить цель визуализации окна по указанному индексу.
    /// @param index индекс вложения для получения. Должен быть в пределах диапазона количества целей визуализации окна.
    /// @return указатель на вложение текстуры в случае успеха; в противном случае 0.
    Texture* WindowAttachmentGet(u8 index);

    /// @return Возвращает указатель на основную цель текстуры глубины.
    Texture* DepthAttachmentGet(u8 index);

    /// @return Возвращает текущий индекс прикрепления окна.
    u8 WindowAttachmentIndexGet();

    /// @brief Возвращает количество вложений, необходимых для целей визуализации на основе окна.
    MAPI u8 WindowAttachmentCountGet();
    
    /// @brief Указывает, поддерживает ли рендерер многопоточность.
    const bool& IsMultithreaded();

    /// @brief Указывает, включен ли предоставленный флаг рендерера. Если передано несколько флагов, все они должны быть установлены, чтобы вернуть значение true.
    /// @param flag проверяемый флаг.
    /// @return True, если флаг(и) установлены; в противном случае false.
    MAPI bool FlagEnabled(RendererConfigFlags flag);

    /// @brief Устанавливает, включены ли включенные флаг(и). Если передано несколько флагов, несколько из них устанавливаются одновременно.
    /// @param flag проверяемый флаг.
    /// @param enabled Указывает, следует ли включать флаг(и).
    MAPI void FlagSetEnabled(RendererConfigFlags flag, bool enabled);

    //////////////////////////////////////////////////////////////////////
    //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////

    /// @brief Создает и назначает буфер, специфичный для бэкэнда рендерера.
    ///
    /// @param buffer Указатель для создания внутреннего буфера.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferCreate(RenderBuffer& buffer);

    /// @brief Создает новый буфер рендеринга для хранения данных для заданной цели/использования. Подкреплено ресурсом буфера, специфичным для бэкэнда рендеринга.
    /// @param type имя буфера рендеринга, используемое для целей отладки.
    /// @param name тип буфера, указывающий его использование (например, данные вершин/индексов, униформы и т. д.)
    /// @param TotalSize общий размер буфера в байтах.
    /// @param UseFreelist указывает, должен ли буфер использовать список свободных значений для отслеживания выделений.
    /// @param buffer ссылка на вновь созданный буфер.
    /// @return True в случае успеха; в противном случае false.
    bool RenderBufferCreate(const char* name, RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer& buffer);

    /// @brief Уничтожает указанный буфер.
    ///
    /// @param buffer Указатель на буфер, который необходимо уничтожить.
    void RenderBufferDestroy(RenderBuffer& buffer);

    /// @brief Связывает заданный буфер с указанным смещением.
    ///
    /// @param buffer Указатель на буфер для привязки.
    /// @param offset Смещение в байтах от начала буфера.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferBind(RenderBuffer& buffer, u64 offset);
    /// @brief Отменяет привязку указанного буфера.
    ///
    /// @param buffer Указатель на буфер, который необходимо отменить.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferUnbind(RenderBuffer& buffer);

    /// @brief Отображает память из указанного буфера в предоставленном диапазоне в блок памяти и возвращает его. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отображения.
    /// @param offset Количество байтов от начала буфера для отображения.
    /// @param size Объем памяти в буфере для отображения.
    /// @returns Отображенный блок памяти. Освобожден и недействителен после отмены отображения.
    void* RenderBufferMapMemory(RenderBuffer& buffer, u64 offset, u64 size);
    /// @brief Отменяет отображение памяти из указанного буфера в предоставленном диапазоне в блок памяти. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для отмены отображения.
    /// @param size Объем памяти в буфере для отмены отображения.
    void RenderBufferUnmapMemory(RenderBuffer& buffer, u64 offset, u64 size);

    /// @brief Очищает буферную память в указанном диапазоне. Должно быть сделано после записи.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для очистки.
    /// @param size Объем памяти в буфере для очистки.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferFlush(RenderBuffer& buffer, u64 offset, u64 size);

    /// @brief Считывает память из предоставленного буфера в указанном диапазоне в выходную переменную.
    /// @param buffer указатель на буфер для чтения.
    /// @param offset количество байтов от начала буфера для чтения.
    /// @param size объем памяти в буфере для чтения.
    /// @param OutMemory указатель на блок памяти для чтения. Должен быть соответствующего размера.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferRead(RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory);

    /// @brief Изменяет размер указанного буфера на NewTotalSize. NewTotalSize должен быть больше текущего размера буфера. Данные из старого внутреннего буфера копируются.
    ///
    /// @param buffer указатель на буфер, размер которого необходимо изменить.
    /// @param NewTotalSize новый размер в байтах. Должен быть больше текущего размера.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferResize(RenderBuffer& buffer, u64 NewTotalSize);

    /// @brief Загружает предоставленные данные в указанный диапазон указанного буфера.
    ///
    /// @param buffer указатель на буфер, в который нужно загрузить данные.
    /// @param offset смещение в байтах от начала буфера.
    /// @param size размер данных в байтах для загрузки.
    /// @param data данные для загрузки.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferLoadRange(RenderBuffer& buffer, u64 offset, u64 size, const void* data);

    /// @brief Копирует данные в указанном диапазоне из исходного буфера в целевой буфер.
    ///
    /// @param source указатель на исходный буфер, из которого копируются данные.
    /// @param SourceOffset смещение в байтах от начала исходного буфера.
    /// @param dest указатель на целевой буфер, в который копируются данные.
    /// @param DestOffset смещение в байтах от начала целевого буфера.
    /// @param size размер копируемых данных в байтах.
    /// @returns True в случае успеха; в противном случае false.
    bool RenderBufferCopyRange(RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size);

    /// @brief Пытается нарисовать содержимое предоставленного буфера с указанным смещением и количеством элементов. 
    /// Предназначено только для использования с буферами вершин и индексов.
    ///
    /// @param buffer указатель на буфер для рисования.
    /// @param offset смещение в байтах от начала буфера.
    /// @param ElementCount количество элементов для рисования.
    /// @param BindOnly только связывает буфер, но не вызывает draw.
    /// @return True в случае успеха; в противном случае false.
    bool RenderBufferDraw(RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly);
};

