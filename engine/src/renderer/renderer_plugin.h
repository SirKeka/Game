#pragma once

#include "renderer_types.h"

// struct RenderingConfig;
struct FrameData;
struct Shader;
struct ShaderConfig;
struct ShaderInstanceResourceConfig;
struct Renderpass;
struct RenderTarget;
struct Texture;
struct TextureMap;
struct RenderTargetAttachment;
struct RenderpassConfig;

/// @brief Общий «интерфейс» для плагина рендеринга. Бэкенд рендеринга — это то, что отвечает за вызовы графического API, такого как Vulkan, OpenGL или DirectX. 
/// Каждый из них должен реализовывать этот интерфейс. Фронтенд взаимодействует только через эту структуру и не имеет никаких знаний о том, как на самом деле все работает на бэкенде.
class MAPI RendererPlugin
{
public:
    u64 FrameNumber{}; // Текущий номер кадра.

    u8 DrawIndex;      // Индекс отрисовки текущего кадра. Обычно совпадает с количеством отправок в очередь на кадр.

    virtual ~RendererPlugin() = default;

    virtual bool Initialize(const RenderingConfig& config, u8& OutWindowRenderTargetCount) = 0;
    virtual void Shutdown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;

    virtual void BeginDebugLabel(const char* LabelText, FVec3 colour) = 0;
    virtual void EndDebugLabel() = 0;

    /// @brief Выполняет процедуры настройки, требуемые в начале кадра. 
    /// @note Ложный результат не обязательно означает сбой. 
    /// Он также может указывать на то, что бэкэнд просто не находится в состоянии, 
    /// способном отрисовать кадр в данный момент, и что его следует попытаться повторить в следующем цикле. 
    /// Конечный кадр не нужно (и не следует) вызывать, если это так.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool PrepareFrame(const FrameData& rFrameData) = 0;

    /// @brief Начинает рендеринг. В каждом кадре должен быть хотя бы один такой элемент и соответствующий ему конец.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool Begin(const FrameData& rFrameData) = 0;

    /// @brief Завершает рендеринг.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return True в случае успеха; в противном случае false.
    virtual bool End(const FrameData& rFrameData) = 0;

    /// @brief Выполняет процедуры, необходимые для отрисовки кадра, например, презентации. Вызывается только после успешного возврата Begin.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return True в случае успеха; в противном случае false.
    virtual bool Present(const FrameData& rFrameData) = 0;

    /// @brief Устанавливает область просмотра рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник области просмотра, который необходимо установить.
    virtual void SetViewport(const Rect2D& rect) = 0;

    /// @brief Сбрасывает область просмотра на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    virtual void ViewportReset() = 0;

    /// @brief Устанавливает ножницы рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник ножниц, который необходимо установить.
    virtual void SetScissor(const Rect2D& rect) = 0;

    /// @brief Сбрасывает ножницы на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    virtual void ScissorReset() = 0;

    /// @brief Включение/отключение тестирования трафарета.
    /// @param enabled Указывает, следует ли включить/отключить тестирование трафарета для последующих отрисовок.
    virtual void SetStencilTestEnabled(bool enabled) = 0;

    /// @brief Включение/отключение тестирования глубины.
    /// @param enabled Указывает, следует ли включить/отключить тестирование глубины для последующих отрисовок.
    virtual void SetDepthTestEnabled(bool enabled) = 0;

    /// @brief Установка ссылки на трафарет для тестирования.
    /// @param reference Ссылка, используемая при тестировании/записи трафарета.
    virtual void SetStencilReference(u32 reference) = 0;

    /// @brief Установить операцию трафарета.
    /// @param FailOp Указывает действие, выполняемое над образцами, не прошедшими тест трафарета.
    /// @param PassOp Указывает действие, выполняемое над образцами, прошедшими тест глубины и тест трафарета.
    /// @param DepthFailOp Указывает действие, выполняемое над образцами, прошедшими тест трафарета, но не прошедшими тест глубины.
    /// @param CompareOp Указывает оператор сравнения, используемый в тесте трафарета.
    virtual void SetStencilOp(RendererStencilOp FailOp, RendererStencilOp PassOp, RendererStencilOp DepthFailOp, RendererCompareOp CompareOp) = 0;

    /// @brief Установить маску сравнения трафарета.
    /// @param CompareMask Новое значение для использования в качестве маски сравнения трафарета.
    virtual void SetStencilCompareMask(u32 CompareMask) = 0;

    /// @brief Установить маску записи трафарета.
    /// @param WriteMask Новое значение для использования в качестве маски записи трафарета.
    virtual void SetStencilWriteMask(u32 WriteMask) = 0;

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
    virtual void TextureWriteData(Texture* texture, u32 offset, u32 size, const u8* pixels, bool IncludeInFrameWorkload) = 0;

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

    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    virtual void Unload(Texture* texture) = 0;

    //////////////////////////////////////////////////////////////////////
    //                            Geometry                              //
    //////////////////////////////////////////////////////////////////////

    virtual bool CreateGeometry(Geometry* geometry) = 0;

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
    virtual bool ShaderCreate(Shader* shader, const ShaderConfig& config, Renderpass* renderpass) = 0;

    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    virtual void ShaderDestroy(Shader* shader) = 0;

    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderInitialize(Shader* shader) = 0;

    /// @brief Перезагружает внутренние компоненты указанного шейдера.
    /// @param shader Указатель на шейдер, который необходимо перезагрузить.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderReload(Shader* shader) = 0;

    /// @brief Указывает, поддерживает ли данный шейдер режим каркасного отображения.
    /// @param shader Константный указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderSupportsWireframe(Shader* shader) = 0;

    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderUse(Shader* shader) = 0;

    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @param NeedsUpdate Указывает, нужно ли обновить шейдерные униформы или достаточно просто привязать их.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyGlobals(Shader* shader, bool NeedsUpdate, const FrameData& rFrameData) = 0;

    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate, const FrameData& rFrameData) = 0;

    /// @brief Запускает загрузку локальных данных униформы на графический процессор.
    /// @param shader указатель на шейдер.
    /// @param rFrameData 
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyLocal(Shader* shader, const FrameData& rFrameData) = 0;

    /// @brief Связывает ресурсы экземпляра для использования и обновления.
    /// @param InstanceID идентификатор экземпляра, который необходимо привязать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderBindInstance(Shader* shader, u32 InstanceID) = 0;

    /// @brief Объединяет глобальные ресурсы для использования и обновления.
    /// @param shader указатель на шейдер.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderBindGlobals(Shader* shader) = 0;

    /// @brief Привязывает локальные ресурсы для использования и обновления.
    /// @param shader указатель на шейдер, чьи локальные ресурсы необходимо привязать.
    /// @return Возвращает True при успешном выполнении; в противном случае False.
    virtual bool ShaderBindLocal(Shader* shader) = 0;

    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param TextureMapCount количество используемых текстурных карт. 
    /// @param maps массив указателей текстурных карт. Должен быть один на текстуру в экземпляре.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderAcquireInstanceResources(Shader* shader, const ShaderInstanceResourceConfig& config, u32& OutInstanceID) = 0;

    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID) = 0;

    /// @brief Получает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстуры, для которой нужно получить ресурсы.
    /// @return true в случае успеха; в противном случае false.
    virtual bool TextureMapResourceAcquire(TextureMap* map) = 0;

    /// @brief Освобождает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстур, из которой необходимо освободить ресурсы.
    virtual void TextureMapDestroy(TextureMap* map) = 0;

    /// @brief 
    /// @param map указатель на карту текстур, из которой необходимо освободить ресурсы.
    /// @return true в случае успеха; в противном случае false.
    virtual bool TextureMapRefresh(TextureMap* map) = 0;
    
    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param ArrayIndex Индекс массива, который необходимо установить, если униформа является массивом. В противном случае игнорируется.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderSetUniform(Shader* shader, struct ShaderUniform* uniform, u32 ArrayIndex, const void* value) = 0;

      //////////////////////////////////////////////////////////////////////
     //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////

    /// @brief Создает и назначает буфер, специфичный для бэкэнда рендерера.
    /// @param buffer Указатель для создания внутреннего буфера.
    /// @returns True в случае успеха; в противном случае false.
    virtual bool RenderBufferCreate(RenderBuffer& buffer) = 0;

    /// @brief Уничтожает указанный буфер.
    /// @param buffer Указатель на буфер, который необходимо уничтожить.
    virtual void RenderBufferDestroy(RenderBuffer& buffer) = 0;

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
    /// @returns true в случае успеха; в противном случае false.
    virtual bool RenderBufferFlush(RenderBuffer& buffer, u64 offset, u64 size) = 0;

    /// @brief Считывает память из предоставленного буфера в указанном диапазоне в выходную переменную.
    /// @param buffer указатель на буфер для чтения.
    /// @param offset количество байтов от начала буфера для чтения.
    /// @param size объем памяти в буфере для чтения.
    /// @param out_memory указатель на блок памяти для чтения. Должен быть соответствующего размера.
    /// @returns true в случае успеха; в противном случае false.
    virtual bool RenderBufferRead(RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory) = 0;

    /// @brief Изменяет размер указанного буфера на NewTotalSize. NewTotalSize должен быть больше текущего размера буфера. Данные из старого внутреннего буфера копируются.
    /// @param buffer указатель на буфер, размер которого необходимо изменить.
    /// @param NewTotalSize новый размер в байтах. Должен быть больше текущего размера.
    /// @returns true в случае успеха; в противном случае false.
    virtual bool RenderBufferResize(RenderBuffer& buffer, u64 NewTotalSize) = 0;

    /// @brief Загружает предоставленные данные в указанный диапазон указанного буфера.
    /// @param buffer указатель на буфер, в который нужно загрузить данные.
    /// @param offset смещение в байтах от начала буфера.
    /// @param size размер данных в байтах для загрузки.
    /// @param data данные для загрузки.
    /// @returns true в случае успеха; в противном случае false.
    virtual bool RenderBufferLoadRange(RenderBuffer& buffer, u64 offset, u64 size, const void* data, bool IncludeInFrameWorkload) = 0;

    /// @brief Копирует данные в указанном диапазоне из исходного буфера в целевой буфер.
    ///
    /// @param source указатель на исходный буфер, из которого копируются данные.
    /// @param SourceOffset смещение в байтах от начала исходного буфера.
    /// @param dest указатель на целевой буфер, в который копируются данные.
    /// @param DestOffset смещение в байтах от начала целевого буфера.
    /// @param size размер копируемых данных в байтах.
    /// @returns true в случае успеха; в противном случае false.
    virtual bool RenderBufferCopyRange(RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size, bool IncludeInFrameWorkload) = 0;

    /// @brief Пытается нарисовать содержимое предоставленного буфера с указанным смещением и количеством элементов. 
    /// Предназначено только для использования с буферами вершин и индексов.
    ///
    /// @param buffer указатель на буфер для рисования.
    /// @param offset смещение в байтах от начала буфера.
    /// @param ElementCount количество элементов для рисования.
    /// @param BindOnly только связывает буфер, но не вызывает draw.
    /// @return true в случае успеха; в противном случае false.
    virtual bool RenderBufferDraw(RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly) = 0;
    
    /// @brief Создает новую цель рендеринга, используя предоставленные данные.
    /// @param AttachmentCount количество вложений.
    /// @param attachments массив вложений.
    /// @param pass указатель на проход рендеринга, с которым связана цель рендеринга.
    /// @param width ширина цели рендеринга в пикселях.
    /// @param height высота цели рендеринга в пикселях.
    /// @param LayerIndex индекс слоя, используемого для прикрепления изображения, если изображение представляет собой текстурный массив.
    /// @param OutTarget указатель для хранения новой созданной цели рендеринга.
    virtual void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, u32 LayerIndex, RenderTarget& OutTarget) = 0;
    
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
    virtual Texture* GetWindowAttachment(u8 index) = 0;
    
    /// @return Возвращает указатель на основную цель текстуры глубины.
    /// @param index Индекс вложения для получения. Должен быть в пределах диапазона количества целевых объектов визуализации окна.
    /// @return Указатель на вложение текстуры в случае успеха; в противном случае nullptr.
    virtual Texture* GetDepthAttachment(u8 index) = 0;
    
    /// @return Возвращает текущий индекс прикрепления окна.
    virtual u8 GetWindowAttachmentIndex() = 0;

    /// @brief Возвращает количество вложений, необходимых для целей визуализации на основе окна.
    virtual u8 GetWindowAttachmentCount() = 0;
    
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

    /// @brief Ожидает, пока бэкенд рендерера полностью завершит всю работу, прежде чем вернуть управление. 
    /// ПРИМЕЧАНИЕ: Это влечет за собой значительные накладные расходы и требует ожидания, поэтому использовать данный механизм следует с осторожностью.
    virtual void WaitForIdle() = 0;
};