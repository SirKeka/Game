#pragma once

#include "renderer_types.h"

// struct RenderingConfig;
// struct Renderpass;
struct RenderTarget;
// struct Texture;
// struct GeometryRenderData;
// #include "resources/shader.h"
// #include "renderbuffer.h"
// struct RenderTargetAttachment;
struct RenderpassConfig;


/// @brief Общий «интерфейс» для плагина рендеринга. Бэкенд рендеринга — это то, что отвечает за вызовы графического API, такого как Vulkan, OpenGL или DirectX. 
/// Каждый из них должен реализовывать этот интерфейс. Фронтенд взаимодействует только через эту структуру и не имеет никаких знаний о том, как на самом деле все работает на бэкенде.
class MAPI RendererPlugin
{
public:
    u64 FrameNumber{};

    virtual ~RendererPlugin() = default;

    virtual bool Initialize(const RenderingConfig& config, u8& OutWindowRenderTargetCount) = 0;
    virtual void ShutDown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;

    /// @brief Выполняет процедуры настройки, требуемые в начале кадра. 
    /// @note Ложный результат не обязательно означает сбой. 
    /// Он также может указывать на то, что бэкэнд просто не находится в состоянии, 
    /// способном отрисовать кадр в данный момент, и что его следует попытаться повторить в следующем цикле. 
    /// Конечный кадр не нужно (и не следует) вызывать, если это так.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool BeginFrame(const FrameData& rFrameData) = 0;

    /// @brief Выполняет процедуры, необходимые для рисования кадра. Следует вызывать только после успешного возврата BeginFrame.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return True в случае успеха; в противном случае false.
    virtual bool EndFrame(const FrameData& rFrameData) = 0;

    /// @brief Устанавливает область просмотра рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник области просмотра, который необходимо установить.
    virtual void ViewportSet(const FVec4& rect) = 0;

    /// @brief Сбрасывает область просмотра на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    virtual void ViewportReset() = 0;

    /// @brief Устанавливает ножницы рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник ножниц, который необходимо установить.
    virtual void ScissorSet(const FVec4& rect) = 0;

    /// @brief Сбрасывает ножницы на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    virtual void ScissorReset() = 0;

    /// @brief Настройте рендерер на использование указанного направления поворота.
    /// @param winding Направление поворота.
    virtual void SetWinding(RendererWinding winding) = 0;

    /// @brief Начинает проход рендеринга с указанной целью.
    /// @param pass указатель на проход рендеринга для начала.
    /// @param target указатель на цель рендеринга для использования.
    /// @return тrue в случае успеха иначе false.
    virtual bool RenderpassBegin(Renderpass* pass, RenderTarget& target) = 0;

    /// @brief Завершает проход рендеринга с указанным идентификатором.
    /// @param pass указатель на проход рендеринга для завершения.
    /// @return тrue в случае успеха иначе false.
    virtual bool RenderpassEnd(Renderpass* pass) = 0;

    //////////////////////////////////////////////////////////////////////
    //                             Texture                              //
    //////////////////////////////////////////////////////////////////////

    /// @brief Загружает данные текстуры в графический процессор.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @param pixels Необработанные данные изображения, которые будут загружены в графический процессор.
    /// @return true в случае успеха, иначе false.
    virtual void Load(const u8* pixels, Texture* texture) = 0;

    /// @brief Загружает новую записываемую текстуру без записи в нее данных.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @return true в случае успеха, иначе false.
    virtual void LoadTextureWriteable(Texture* texture) = 0;

    /// @brief Изменяет размер текстуры. На этом уровне нет проверки возможности записи текстуры. 
    /// Внутренние ресурсы уничтожаются и воссоздаются при новом разрешении. Данные потеряны, и их необходимо перезагрузить.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param NewWidth новая ширина в пикселях.
    /// @param NewHeight новая высота в пикселях.
    virtual void TextureResize(Texture* texture, u32 NewWidth, u32 NewHeight) = 0;

    /// @brief Записывает данные в предоставленную текстуру. ПРИМЕЧАНИЕ: На этом уровне это может быть как записываемая, 
    /// так и незаписываемая текстура, поскольку она также обрабатывает начальную загрузку текстуры. Сама система текстур 
    /// должна отвечать за блокировку запросов на запись в недоступные для записи текстуры.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param pixels необработанные данные изображения, которые необходимо записать.
    virtual void TextureWriteData(Texture* texture, u32 offset, u32 size, const u8* pixels) = 0;

    /// @brief Считывает указанные данные из предоставленной текстуры.
    /// @param texture Указатель на текстуру для чтения.
    /// @param offset Смещение в байтах от начала данных для чтения.
    /// @param size Количество байтов для чтения.
    /// @param OutMemory Указатель на блок памяти для записи считанных данных.
    virtual void TextureReadData(Texture* texture, u32 offset, u32 size, void** OutMemory) = 0;

    /// @brief Считывает пиксель из предоставленной текстуры с указанной координатой x/y.
    /// @param texture Указатель на текстуру для чтения.
    /// @param x Координата x пикселя.
    /// @param y Координата y пикселя.
    /// @param OutRgba Указатель на массив u8 для хранения данных пикселей (должен быть sizeof(u8) * 4)
    virtual void TextureReadPixel(Texture* texture, u32 x, u32 y, u8** OutRgba) = 0;

    virtual void* TextureCopyData(const Texture* texture) = 0;

    //////////////////////////////////////////////////////////////////////
    //                            Geometry                              //
    //////////////////////////////////////////////////////////////////////

    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    virtual void Unload(Texture* texture) = 0;

    virtual bool CreateGeometry(Geometry* geometry) = 0;

    /// @brief Получает внутренние ресурсы, специфичные для API рендерера, для заданной геометрии и загружает данные в графический процессор.
    /// @param geometry указатель на геометрию, которую нужно инициализировать.
    /// @param VertexOffset смещение в байтах от начала данных вершин геометрии.
    /// @param VertexSize количество загружаемых данных вершин в байтах.
    /// @param IndexOffset смещение в байтах от начала данных индекса геометрии.
    /// @param IndexSize количество загружаемых данных индекса в байтах.
    /// @return True в случае успешного выполнения; в противном случае false.
    virtual bool Load(Geometry* geometry, u32 VertexOffset, u32 VertexSize, u32 IndexOffset, u32 IndexSize) = 0;
    
    virtual void Unload(Geometry* gid) = 0;

    /// @brief Обновляет данные вершин в заданной геометрии предоставленными данными в заданном диапазоне.
    /// @param geometry Указатель на геометрию, которую нужно создать.
    /// @param offset Смещение в байтах для обновления. 0, если обновление с начала.
    /// @param VertexCount Количество вершин, которые будут обновлены.
    /// @param vertices Данные вершин.
    virtual void GeometryVertexUpdate(Geometry* geometry, u32 offset, u32 VertexCount, void* vertices) = 0;

    /// @brief Рисует заданную геометрию. Должен вызываться только внутри прохода рендеринга, внутри кадра.
    /// @param data данные рендеринга геометрии, которая должна быть нарисована.
    virtual void DrawGeometry(const GeometryRenderData& data) = 0;

    //////////////////////////////////////////////////////////////////////
    //                              Shader                              //
    //////////////////////////////////////////////////////////////////////

    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    /// @param shader указатель на шейдер.
    /// @param config константная ссылка на конфигурацию шейдера.
    /// @param renderpass указатель на проход рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    virtual bool Load(Shader *shader, const ShaderConfig& config, Renderpass* renderpass, const DArray<Shader::Stage>& stages, const DArray<MString>& StageFilenames) = 0;

    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    virtual void Unload(Shader* shader) = 0;

    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderInitialize(Shader* shader) = 0;

    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderUse(Shader* shader) = 0;

    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyGlobals(Shader* shader, bool NeedsUpdate) = 0;

    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate) = 0;

    /// @brief Связывает ресурсы экземпляра для использования и обновления.
    /// @param InstanceID идентификатор экземпляра, который необходимо привязать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderBindInstance(Shader* shader, u32 InstanceID) = 0;

    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param TextureMapCount количество используемых текстурных карт. 
    /// @param maps массив указателей текстурных карт. Должен быть один на текстуру в экземпляре.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderAcquireInstanceResources(Shader* shader, u32 TextureMapCount, struct TextureMap** maps, u32& OutInstanceID) = 0;

    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID) = 0;

    /// @brief Получает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстуры, для которой нужно получить ресурсы.
    /// @return true в случае успеха; в противном случае false.
    virtual bool TextureMapAcquireResources(TextureMap* map) = 0;

    /// @brief Освобождает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстур, из которой необходимо освободить ресурсы.
    virtual void TextureMapReleaseResources(TextureMap* map) = 0;
    
    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    virtual bool SetUniform(Shader* shader, struct Shader::Uniform* uniform, const void* value) = 0;

    //////////////////////////////////////////////////////////////////////
    //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////

    /// @brief Создает и назначает буфер, специфичный для бэкэнда рендерера.
    /// @param buffer Указатель для создания внутреннего буфера.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferCreateInternal(RenderBuffer& buffer) = 0;

    /// @brief Создает новый буфер рендеринга для хранения данных для заданной цели/использования. Подкрепленный ресурсом буфера, специфичным для бэкэнда рендеринга.
    /// @param name имя буфера рендеринга, используемого для целей отладки.
    /// @param type тип буфера, указывающий его использование (т. е. данные вершин/индексов, униформы и т. д.)
    /// @param TotalSize общий размер буфера в байтах.
    /// @param UseFreelist указывает, должен ли буфер использовать список свободных страниц для отслеживания выделений.
    /// @param buffer указатель для хранения вновь созданного буфера.
    /// @return true в случае успеха, иначе false.
    virtual bool RenderBufferCreate(const char* name, RenderBufferType type, u64 TotalSize, bool UseFreelist, RenderBuffer &buffer) = 0;

    /// @brief Уничтожает указанный буфер.
    /// @param buffer Указатель на буфер, который необходимо уничтожить.
    virtual void RenderBufferDestroyInternal(RenderBuffer& buffer) = 0;

    /// @brief Связывает заданный буфер с указанным смещением.
    /// @param buffer Указатель на буфер для привязки.
    /// @param offset Смещение в байтах от начала буфера.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferBind(RenderBuffer& buffer, u64 offset) = 0;
    /// @brief Отменяет привязку указанного буфера.
    /// @param buffer Указатель на буфер, который необходимо отменить.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferUnbind(RenderBuffer& buffer) = 0;

    /// @brief Отображает память из указанного буфера в предоставленном диапазоне в блок памяти и возвращает его. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отображения.
    /// @param offset Количество байтов от начала буфера для отображения.
    /// @param size Объем памяти в буфере для отображения.
    /// @returns Отображенный блок памяти. Освобожден и недействителен после отмены отображения.
    virtual void* RenderBufferMapMemory(RenderBuffer& buffer, u64 offset, u64 size) = 0;
    
    /// @brief Отменяет отображение памяти из указанного буфера в предоставленном диапазоне в блок памяти. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для отмены отображения.
    /// @param size Объем памяти в буфере для отмены отображения.
    virtual void RenderBufferUnmapMemory(RenderBuffer& buffer, u64 offset, u64 size) = 0;

    /// @brief Очищает буферную память в указанном диапазоне. Должно быть сделано после записи.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для очистки.
    /// @param size Объем памяти в буфере для очистки.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferFlush(RenderBuffer& buffer, u64 offset, u64 size) = 0;

    /// @brief Считывает память из предоставленного буфера в указанном диапазоне в выходную переменную.
    /// @param buffer указатель на буфер для чтения.
    /// @param offset количество байтов от начала буфера для чтения.
    /// @param size объем памяти в буфере для чтения.
    /// @param out_memory указатель на блок памяти для чтения. Должен быть соответствующего размера.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferRead(RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory) = 0;

    /// @brief Изменяет размер указанного буфера на NewTotalSize. NewTotalSize должен быть больше текущего размера буфера. Данные из старого внутреннего буфера копируются.
    ///
    /// @param buffer указатель на буфер, размер которого необходимо изменить.
    /// @param NewTotalSize новый размер в байтах. Должен быть больше текущего размера.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferResize(RenderBuffer& buffer, u64 NewTotalSize) = 0;

    /// @brief Загружает предоставленные данные в указанный диапазон указанного буфера.
    ///
    /// @param buffer указатель на буфер, в который нужно загрузить данные.
    /// @param offset смещение в байтах от начала буфера.
    /// @param size размер данных в байтах для загрузки.
    /// @param data данные для загрузки.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferLoadRange(RenderBuffer& buffer, u64 offset, u64 size, const void* data) = 0;

    /// @brief Копирует данные в указанном диапазоне из исходного буфера в целевой буфер.
    ///
    /// @param source указатель на исходный буфер, из которого копируются данные.
    /// @param SourceOffset смещение в байтах от начала исходного буфера.
    /// @param dest указатель на целевой буфер, в который копируются данные.
    /// @param DestOffset смещение в байтах от начала целевого буфера.
    /// @param size размер копируемых данных в байтах.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferCopyRange(RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size) = 0;

    /// @brief Пытается нарисовать содержимое предоставленного буфера с указанным смещением и количеством элементов. 
    /// Предназначено только для использования с буферами вершин и индексов.
    ///
    /// @param buffer указатель на буфер для рисования.
    /// @param offset смещение в байтах от начала буфера.
    /// @param ElementCount количество элементов для рисования.
    /// @param BindOnly только связывает буфер, но не вызывает draw.
    /// @return True в случае успеха; в противном случае false.
    virtual bool RenderBufferDraw(RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly) = 0;
    
    /// @brief Создает новую цель рендеринга, используя предоставленные данные.
    /// @param AttachmentCount количество вложений.
    /// @param attachments массив вложений.
    /// @param pass указатель на проход рендеринга, с которым связана цель рендеринга.
    /// @param width ширина цели рендеринга в пикселях.
    /// @param height высота цели рендеринга в пикселях.
    /// @param OutTarget указатель для хранения новой созданной цели рендеринга.
    virtual void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, RenderTarget& OutTarget) = 0;
    
    /// @brief Уничтожает предоставленную цель рендеринга.
    /// @param target указатель на цель рендеринга, которую нужно уничтожить.
    /// @param FreeInternalMemory указывает, следует ли освободить внутреннюю память.
    virtual void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory) = 0;
    
    /// @brief Создает новый проход рендеринга.
    /// @param config ссылка на конфигурацию, которая будет использоваться при создании прохода рендеринга.
    /// @param OutRenderpass указатель на общий проход рендеринга.
    /// @param flag если true, то данные из config будут скопированы в OutRenderpass
    virtual bool RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass, bool copy) = 0;
    
    /// @brief Уничтожает указанный renderpass.
    /// @param OutRenderpass указатель на renderpass, который необходимо уничтожить.
    virtual void RenderpassDestroy(Renderpass* OutRenderpass) = 0;
    
    /// @brief Пытается получить цель визуализации окна по указанному индексу.
    /// @param index индекс вложения для получения. Должен быть в пределах диапазона количества целей визуализации окна.
    /// @return указатель на вложение текстуры в случае успеха; в противном случае 0.
    virtual Texture* WindowAttachmentGet(u8 index) = 0;
    
    /// @return Возвращает указатель на основную цель текстуры глубины.
    /// @param index Индекс вложения для получения. Должен быть в пределах диапазона количества целевых объектов визуализации окна.
    /// @return Указатель на вложение текстуры в случае успеха; в противном случае nullptr.
    virtual Texture* DepthAttachmentGet(u8 index) = 0;
    
    /// @return Возвращает текущий индекс прикрепления окна.
    virtual u8 WindowAttachmentIndexGet() = 0;

    /// @brief Возвращает количество вложений, необходимых для целей визуализации на основе окна.
    virtual u8 WindowAttachmentCountGet() = 0;
    
    /// @brief Указывает, поддерживает ли рендерер многопоточность.
    virtual const bool& IsMultithreaded() = 0;

    /// @brief Указывает, включен ли предоставленный флаг рендерера. Если передано несколько флагов, все они должны быть установлены, чтобы вернуть значение true.
    /// @param flag проверяемый флаг.
    /// @return True, если флаг(и) установлены; в противном случае false.
    virtual bool FlagEnabled(RendererConfigFlags flag) = 0;

    /// @brief Устанавливает, включены ли включенные флаг(и). Если передано несколько флагов, несколько из них устанавливаются одновременно.
    /// @param flag проверяемый флаг.
    /// @param enabled указывает, следует ли включать флаг(и).
    virtual void FlagSetEnabled(RendererConfigFlags flag, bool enabled) = 0;
    };