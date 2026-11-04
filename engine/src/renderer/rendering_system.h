/// @file renderer.h
/// @author 
/// @brief Интерфейс рендеринга — единственное, что видит остальная часть движка. 
/// Он отвечает за передачу любых данных в/из бэкэнда рендеринга независимым способом.
/// @version 1.0
/// @date 
///
/// @copyright 
#pragma once
#include "core/mvar.h"
#include "renderer_plugin.h"

using PFN_PluginCreate = RendererPlugin*(*)(void*, u64*);
struct Viewport;

class RenderingSystem
{
    RenderBuffer GeometryVertexBuffer;  // Буфер вершин объекта, используемый для хранения вершин геометрии.
    RenderBuffer GeometryIndexBuffer;   // Буфер индекса объекта, используемый для хранения индексов геометрии.
    RendererPlugin* ptrRenderer;
    Viewport* ActiveViewport;
    u32 FramebufferWidth;               // Текущая ширина буфера кадра окна.
    u32 FramebufferHeight;              // Текущая высота буфера кадра окна.
    u8 WindowRenderTargetCount;         // Количество целей рендеринга. Обычно совпадает с количеством изображений swapchain.

public:
    struct Config {
        const char *ApplicationName;
        PFN_PluginCreate PluginCreate;
    };
    
    /// @brief Инициализирует интерфейс/систему рендеринга.
    constexpr RenderingSystem(Config& config): 
    GeometryVertexBuffer("renderbuffer_vertexbuffer_globalgeometry", RenderBufferType::Vertex, RenderBuffer::AllocationType::FreeList, sizeof(Vertex3D) * 1024 * 1024 * 10), 
    GeometryIndexBuffer("renderbuffer_indexbuffer_globalgeometry", RenderBufferType::Index, RenderBuffer::AllocationType::FreeList, sizeof(u32) * 1024 * 1024 * 100), 
    ptrRenderer(config.PluginCreate(this + 1, nullptr)), ActiveViewport(nullptr), FramebufferWidth(1280), FramebufferHeight(720), WindowRenderTargetCount() 
    {
        RenderingConfig RConfig{
            .ApplicationName = config.ApplicationName
        };

        // ЗАДАЧА: предоставить это приложению для настройки.
        RConfig.flags = RenderingConfigFlagBits::VsyncEnabledBit | RenderingConfigFlagBits::PowerSavingBit;
        //  ПРИМЕЧАНИЕ: Чтобы включить проверку, раскомментируйте эту строку.
        RConfig.flags |= RenderingConfigFlagBits::EnableValidation;
        
        // Создайте vsync mvar
        MVar::CreateInt("vsync", (RConfig.flags & RenderingConfigFlagBits::VsyncEnabledBit) ? 1 : 0);
                
        if (!ptrRenderer->Initialize(RConfig, WindowRenderTargetCount)) {
            MERROR("Систему рендеринга не удалось инициализировать. Выключение.");
            return;
        }

        // Создать буферы

        // Буфер вершин геометрии
        if (!ptrRenderer->RenderBufferCreate(GeometryVertexBuffer)) {
            MERROR("Ошибка создания буфера вершин.");
            return;
        }
        ptrRenderer->RenderBufferBind(GeometryVertexBuffer, 0);
        
        // Буфер индексов геометрии
        if (!ptrRenderer->RenderBufferCreate(GeometryIndexBuffer)) {
            MERROR("Ошибка создания буфера индексов.");
            return;
        }
        ptrRenderer->RenderBufferBind(GeometryIndexBuffer, 0);
    }

    MAPI RenderBuffer* GetRenderbuffer(RenderBufferType type);
    
    void Shutdown();

    /// @brief Обрабатывает события изменения размера.
    /// @param width Новая ширина окна.
    /// @param height Новая высота окна.
    void OnResized(u16 width, u16 height);

    /// @brief Рисует следующий кадр, используя данные, предоставленные в пакете рендеринга.
    /// @param packet константная ссылка на пакет рендеринга, который содержит данные о том, что должно быть визуализировано.
    /// @param rFrameData константная ссылка на данные текущего кадра
    /// @return true в случае успеха; в противном случае false.
    MAPI bool PrepareFrame(FrameData& rFrameData);

    /// @brief Начинает рендеринг. В каждом кадре должен быть хотя бы один такой элемент и соответствующий ему конец.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Begin(const FrameData& rFrameData) {
        return ptrRenderer ? ptrRenderer->Begin(rFrameData) : false;
    }

    /// @brief Завершает рендеринг.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return True в случае успеха; в противном случае false.
    MAPI bool End(FrameData& rFrameData);

    /// @brief Выполняет процедуры, необходимые для отрисовки кадра, например, презентации. Вызывается только после успешного возврата Begin.
    /// @param rFrameData константная ссылка на данные текущего кадра.
    /// @return True в случае успеха; в противном случае false.
    MAPI bool Present(const FrameData& rFrameData);

    /// @brief Обозначает начало секции команд, сгруппированных под определенным именем и цветом. 
    /// В сборках, не предназначенных для отладки, эта операция не выполняет никаких действий. 
    /// ПРИМЕЧАНИЕ: Реализация этой функции может различаться в зависимости от бэкенда рендеринга или вовсе отсутствовать.
    /// @param LabelText текст для использования на этикетке.
    /// @param colour цвет, который будет использоваться для этикетки.
    MAPI void BeginDebugLabel(const char* LabelText, FVec3 colour);

    /// @brief Завершает последнюю секцию отладочных команд. В сборках без отладочной информации не выполняет никаких действий. 
    /// ПРИМЕЧАНИЕ: Реализация этой функции будет различаться в зависимости от бэкенда рендеринга или может отсутствовать вовсе.
    MAPI void EndDebugLabel();
 
    /// @brief Загружает данные текстуры в графический процессор.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @param pixels Необработанные данные изображения, которые будут загружены в графический процессор.
    /// @return true в случае успеха, иначе false.
    MAPI void Load(const u8* pixels, Texture* texture) {
        return ptrRenderer->Load(pixels, texture);
    }

    /// @brief Загружает новую записываемую текстуру без записи в нее данных.
    /// @param texture указатель на текстуру которую нужно загрузить.
    /// @return true в случае успеха, иначе false.
    MAPI void LoadTextureWriteable(Texture* texture) {
        ptrRenderer->LoadTextureWriteable(texture);
    }

    /// @brief Изменяет размер текстуры. На этом уровне нет проверки возможности записи текстуры. 
    /// Внутренние ресурсы уничтожаются и воссоздаются при новом разрешении. Данные потеряны, и их необходимо перезагрузить.
    /// @param texture указатель на текстуру, размер которой нужно изменить.
    /// @param NewWidth новая ширина в пикселях.
    /// @param NewHeight новая высота в пикселях.
    MAPI void TextureResize(Texture* texture, u32 NewWidth, u32 NewHeight) {
        ptrRenderer->TextureResize(texture, NewWidth, NewHeight);
    }

    /// @brief Записывает данные в предоставленную текстуру. ПРИМЕЧАНИЕ: На этом уровне это может быть как записываемая, 
    /// так и незаписываемая текстура, поскольку она также обрабатывает начальную загрузку текстуры. Сама система текстур 
    /// должна отвечать за блокировку запросов на запись в недоступные для записи текстуры.
    /// @param texture указатель на текстуру, в которую нужно записать.
    /// @param offset смещение в байтах от начала записываемых данных.
    /// @param size количество байтов, которые необходимо записать.
    /// @param pixels необработанные данные изображения, которые необходимо записать.
    MAPI void TextureWriteData(Texture* texture, u32 offset, u32 size, const u8* pixels) {
        ptrRenderer->TextureWriteData(texture, offset, size, pixels, false);
    }

    /// @brief Считывает указанные данные из предоставленной текстуры.
    /// @param texture Указатель на текстуру для чтения.
    /// @param offset Смещение в байтах от начала данных для чтения.
    /// @param size Количество байтов для чтения.
    /// @param OutMemory Указатель на блок памяти для записи считанных данных.
    MAPI void TextureReadData(Texture* texture, u32 offset, u32 size, void** OutMemory) {
        ptrRenderer->TextureReadData(texture, offset, size, OutMemory);
    }

    /// @brief Считывает пиксель из предоставленной текстуры с указанной координатой x/y.
    /// @param texture Указатель на текстуру для чтения.
    /// @param x Координата x пикселя.
    /// @param y Координата y пикселя.
    /// @param OutRgba Указатель на массив u8 для хранения данных пикселей (должен быть sizeof(u8) * 4)
    MAPI void TextureReadPixel(Texture* texture, u32 x, u32 y, u8** OutRgba) {
        ptrRenderer->TextureReadPixel(texture, x, y, OutRgba);
    }

    /// @brief Копирует данные структуры и возвращает указатель на них
    /// @param texture текстура данные которой нужно скопировать
    /// @return укахатель на данные
    void* TextureCopyData(const Texture* texture) {
        return ptrRenderer->TextureCopyData(texture);
    }

    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    MAPI void Unload(Texture* texture) {
        ptrRenderer->Unload(texture);
    }

    /// @brief Загружает геометрию, копируя предоставленные данные и настраивая структуру данных.------------------------------------------------------------------------------------
    /// @param geometry указатель на геометрию, которую требуется загрузить.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool CreateGeometry(Geometry* geometry) {
        return ptrRenderer->CreateGeometry(geometry);
    }

    /// @brief Получает ресурсы графического процессора и загружает геометрические данные.
    /// @param geometry указатель на геометрию для загрузки.
    /// @return True в случае успешного выполнения; в противном случае — false.
    MAPI bool Load(Geometry* geometry);
    
    /// @brief Обновляет данные вершин в заданной геометрии предоставленными данными в заданном диапазоне.
    /// @param geometry Указатель на геометрию, которую нужно создать.
    /// @param offset Смещение в байтах для обновления. 0, если обновление с начала.
    /// @param VertexCount Количество вершин, которые будут обновлены.
    /// @param vertices Данные вершин.
    MAPI void GeometryVertexUpdate(Geometry* geometry, u32 offset, bool IncludeInFrameWorkload = true);

    /// @brief Уничтожает заданную геометрию, освобождая ресурсы графического процессора.-------------------------------------------------------------
    /// @param gid указатель на геометрию, которую нужно уничтожить.
    MAPI void Unload(Geometry* geometry);

    /// @brief Рисует заданную геометрию. Должен вызываться только внутри прохода рендеринга, внутри кадра.
    /// @param data Данные рендеринга геометрии, которая должна быть нарисована.
    MAPI void DrawGeometry(const GeometryRenderData& data);

    /// @brief Устанавливает область просмотра рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник области просмотра, который необходимо установить.
    MAPI void ViewportSet(const Rect2D& rect) {
        ptrRenderer->SetViewport(rect);
    }

    /// @brief Сбрасывает область просмотра на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    MAPI void ViewportReset() {
        ptrRenderer->ViewportReset();
    }

    /// @brief Устанавливает ножницы рендерера на заданный прямоугольник. Должно быть сделано в проходе рендеринга.
    /// @param rect Прямоугольник ножниц, который необходимо установить.
    MAPI void ScissorSet(const Rect2D& rect) {
        ptrRenderer->SetScissor(rect);
    }

    /// @brief Сбрасывает ножницы на значение по умолчанию, которое соответствует окну приложения.
    /// Должно быть сделано в проходе рендеринга.
    MAPI void ScissorReset() {
        ptrRenderer->ScissorReset();
    }

    /// @brief Включить/выключить проверку глубины.
    /// @param enabled Указывает, следует ли включать/выключать проверку глубины для последующих розыгрышей.
    MAPI void SetDepthTestEnabled(bool enabled) {
        ptrRenderer->SetDepthTestEnabled(enabled);
    }
    
    /// @brief Установить маску сравнения трафарета.
    /// @param CompareMask новое значение, которое будет использоваться в качестве маски сравнения трафарета.
    MAPI void SetStencilCompareMask(u32 CompareMask) {
        ptrRenderer->SetStencilCompareMask(CompareMask);
    }
    
    /// @brief Настройка трафаретной операции.
    /// @param FailOp указывает действие, выполняемое с образцами, не прошедшими трафаретный тест.
    /// @param PassOp Указывает действие, выполняемое с образцами, прошедшими как тест на глубину, так и тест на трафарет.
    /// @param DepthFailOp Указывает действие, выполняемое с образцами, прошедшими тест на трафарет, но не прошедшими тест на глубину.
    /// @param CompareOp Указывает оператор сравнения, используемый в трафаретном тесте.
    MAPI void SetStencilOp(RendererStencilOp FailOp, RendererStencilOp PassOp, RendererStencilOp DepthFailOp, RendererCompareOp CompareOp) {
        ptrRenderer->SetStencilOp(FailOp, PassOp, DepthFailOp, CompareOp);
    }
    
    /// @brief Установите эталон трафарета для тестирования.
    /// @param reference Эталонный параметр, используемый при тестировании/записи трафарета.
    MAPI void SetStencilReference(u32 reference) {
        ptrRenderer->SetStencilReference(reference);
    }

    /// @brief Включить/отключить проверку трафарета.
    /// @param enabled Указывает, следует ли включать/отключать проверку трафарета для последующих отрисовок.
    MAPI void SetStencilTestEnabled(bool enabled) {
        ptrRenderer->SetStencilTestEnabled(enabled);
    }
    
    /// @brief Установить маску записи трафарета.
    /// @param WriteMask новое значение, которое будет использоваться в качестве маски записи трафарета.
    MAPI void SetStencilWriteMask(u32 WriteMask) {
        ptrRenderer->SetStencilWriteMask(WriteMask);
    }

    /// @brief Настройте рендерер на использование указанного направления поворота.
    /// @param winding Направление поворота.
    MAPI void SetWinding(RendererWinding winding) {
        ptrRenderer->SetWinding(winding);
    }

    /// @brief Начинает проход рендеринга с указанной целью.
    /// @param pass указатель на проход рендеринга для начала.
    /// @param target указатель на цель рендеринга для использования.
    /// @return тrue в случае успеха иначе false.
    MAPI bool RenderpassBegin(Renderpass* pass, RenderTarget& target) {
        return ptrRenderer->RenderpassBegin(pass, target);
    }

    /// @brief Завершает проход рендеринга с указанным идентификатором.
    /// @param pass указатель на проход рендеринга для завершения.
    /// @return тrue в случае успеха иначе false.
    MAPI bool RenderpassEnd(Renderpass* pass) {
        return ptrRenderer->RenderpassEnd(pass);
    }

    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.---------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param config константный указатель на конфигурацию шейдера.
    /// @param renderpass указатель на renderpass.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderCreate(Shader* shader, const ShaderConfig& config, Renderpass* renderpass);
    
    /// @brief Уничтожает данный шейдер и освобождает все имеющиеся в нем ресурсы.--------------------------------------------------------------------
    /// @param shader указатель на шейдер, который нужно уничтожить.
    MAPI void ShaderDestroy(Shader* shader);

    /// @brief Инициализирует настроенный шейдер. Будет автоматически уничтожен, если этот шаг не удастся.--------------------------------------------
    /// Должен быть вызван после Shader::Create().
    /// @param shader указатель на шейдер, который необходимо инициализировать.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderInitialize(Shader* shader) {
        return ptrRenderer->ShaderInitialize(shader);
    }

    /// @brief Использует заданный шейдер, активируя его для обновления атрибутов, униформы и т. д., а также для использования в вызовах отрисовки.---
    /// @param shader указатель на используемый шейдер.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderUse(Shader* shader) {
        return ptrRenderer->ShaderUse(shader);
    }

    /// @brief Применяет глобальные данные к универсальному буферу.-----------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить глобальные данные.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderApplyGlobals(Shader* shader, bool NeedsUpdate, const FrameData &rFrameData) {
        return ptrRenderer->ShaderApplyGlobals(shader, NeedsUpdate, rFrameData);
    }

    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate, const FrameData &rFrameData) {
        return ptrRenderer->ShaderApplyInstance(shader, NeedsUpdate, rFrameData);
    }

    /// @brief Объединяет глобальные ресурсы для использования и обновления.
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderBindGlobals(Shader* shader) {
        return ptrRenderer->ShaderBindGlobals(shader);
    }

    /// @brief Связывает ресурсы экземпляра для использования и обновления.
    /// @param InstanceID идентификатор экземпляра, который необходимо привязать.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderBindInstance(Shader* shader, u32 InstanceID = 0) {
        return ptrRenderer->ShaderBindInstance(shader, InstanceID);
    }

    /// @brief Привязывает локальные ресурсы для использования и обновления.
    /// @param shader указатель на шейдер, чьи локальные ресурсы необходимо привязать.
    /// @return Возвращает True при успешном выполнении; в противном случае False.
    MAPI bool ShaderBindLocal(Shader* shader) {
        return ptrRenderer->ShaderBindLocal(shader);
    }

    /// @brief Запускает загрузку локальных данных униформы на графический процессор.
    /// @param shader указатель на шейдер.
    /// @param rFrameData 
    /// @return 
    bool ShaderApplyLocal(Shader* shader, const FrameData& rFrameData) {
        return ptrRenderer->ShaderApplyLocal(shader, rFrameData);
    }

    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param TextureMapCount количество используемых текстурных карт.
    /// @param maps массив указателей текстурных карт. Должен быть один на текстуру в экземпляре.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderAcquireInstanceResources(Shader* shader, const ShaderInstanceResourceConfig &config, u32& OutInstanceID) {
        return ptrRenderer->ShaderAcquireInstanceResources(shader, config, OutInstanceID);
    }

    /// @brief Освобождает внутренние ресурсы уровня экземпляра для данного идентификатора экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, из которого необходимо освободить ресурсы.
    /// @param InstanceID идентификатор экземпляра, ресурсы которого должны быть освобождены.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderReleaseInstanceResources(Shader* shader, u32 InstanceID) {
        return ptrRenderer->ShaderReleaseInstanceResources(shader, InstanceID);
    }

    /// @brief Перезагружает указанный шейдер.
    /// @param shader шейдер, который нужно перезагрузить
    /// @return Возвращает true при успешном выполнении, иначе — false.
    MAPI bool ShaderReload(Shader* shader);

    /// @brief Устанавливает униформу данного шейдера на указанное значение.--------------------------------------------------------------------------
    /// @param shader указатель на шейдер.
    /// @param uniform постоянный указатель на униформу.
    /// @param value указатель на значение, которое необходимо установить.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderSetUniform(Shader* shader, ShaderUniform* uniform, u32 ArrayIndex, const void* value) {
        return ptrRenderer->ShaderSetUniform(shader, uniform, ArrayIndex, value);
    }

    /// @brief Предпринимает попытку включить режим каркасного отображения (wireframe) для указанного шейдера. 
    /// Если используемый бэкенд или сам шейдер не поддерживают этот режим, попытка включения завершится неудачей. 
    //  Отключение режима всегда проходит успешно.
    /// @param shader указатель на используемый шейдер.
    /// @param WireframeEnabled указывает, следует ли включить режим каркасного отображения.
    /// @return true в случае успеха, иначе false.
    MAPI bool ShaderSetWireframe(Shader* shader, bool WireframeEnabled);

    /// @brief Получает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстуры, для которой нужно получить ресурсы.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool TextureMapAcquireResources(TextureMap* map) {
        return ptrRenderer->TextureMapResourceAcquire(map);
    }

    /// @brief Освобождает внутренние ресурсы для данной карты текстур.
    /// @param map указатель на карту текстур, из которой необходимо освободить ресурсы.
    MAPI void TextureMapReleaseResources(TextureMap* map) {
        ptrRenderer->TextureMapDestroy(map);
    }

    /// @brief Создает новую цель рендеринга, используя предоставленные данные.
    /// @param AttachmentCount количество вложений (указателей текстур).
    /// @param attachments массив вложений (указателей текстур).
    /// @param pass указатель на проход рендеринга, с которым связана цель рендеринга.
    /// @param width ширина цели рендеринга в пикселях.
    /// @param height высота цели рендеринга в пикселях.
    /// @param OutTarget указатель для хранения новой созданной цели рендеринга.
    MAPI void RenderTargetCreate(u8 AttachmentCount, RenderTargetAttachment* attachments, Renderpass* pass, u32 width, u32 height, u32 LayerIndex, RenderTarget& OutTarget) {
        ptrRenderer->RenderTargetCreate(AttachmentCount, attachments, pass, width, height, LayerIndex, OutTarget);
    }
    
    /// @brief Уничтожает предоставленную цель рендеринга.
    /// @param target указатель на цель рендеринга, которую нужно уничтожить.
    /// @param FreeInternalMemory указывает, следует ли освободить внутреннюю память.
    MAPI void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory) {
        ptrRenderer->RenderTargetDestroy(target, FreeInternalMemory);
    }

    /// @brief Создает новый проход рендеринга.
    /// @param config Константная ссылка на конфигурацию, которая будет использоваться при создании прохода рендеринга.
    /// @param OutRenderpass указатель на общий проход рендеринга.
    /// @param copy указывает нужно ли копировать данные из config в OutRenderpass
    MAPI MINLINE bool RenderpassCreate(RenderpassConfig& config, Renderpass& OutRenderpass, bool copy = true) {
        return ptrRenderer->RenderpassCreate(config, OutRenderpass, copy);
    }
    
    /// @brief Уничтожает указанный renderpass.
    /// @param OutRenderpass указатель на renderpass, который необходимо уничтожить.
    MAPI void RenderpassDestroy(Renderpass* OutRenderpass) {
        ptrRenderer->RenderpassDestroy(OutRenderpass);
    }

    /// @brief Пытается получить цель визуализации окна по указанному индексу.
    /// @param index индекс вложения для получения. Должен быть в пределах диапазона количества целей визуализации окна.
    /// @return указатель на вложение текстуры в случае успеха; в противном случае 0.
    struct Texture* GetWindowAttachment(u8 index) {
        return ptrRenderer->GetWindowAttachment(index);
    }

    /// @return Возвращает указатель на основную цель текстуры глубины.
    struct Texture* GetDepthAttachment(u8 index) {
        return ptrRenderer->GetDepthAttachment(index);
    }

    /// @return Возвращает текущий индекс прикрепления окна.
    u8 GetWindowAttachmentIndex() {
        return ptrRenderer->GetWindowAttachmentIndex();
    }

    /// @brief Возвращает количество вложений, необходимых для целей визуализации на основе окна.
    MAPI u8 GetWindowAttachmentCount() {
        return ptrRenderer->GetWindowAttachmentCount();
    }
    
    /// @brief Указывает, поддерживает ли рендерер многопоточность.
    const bool& IsMultithreaded() {
        return ptrRenderer->IsMultithreaded();
    }

    /// @brief Указывает, включен ли предоставленный флаг рендерера. Если передано несколько флагов, все они должны быть установлены, чтобы вернуть значение true.
    /// @param flag проверяемый флаг.
    /// @return True, если флаг(и) установлены; в противном случае false.
    MAPI bool FlagEnabled(RendererConfigFlags flag) {
        return ptrRenderer->FlagEnabled(flag);
    }

    /// @brief Устанавливает, включены ли включенные флаг(и). Если передано несколько флагов, несколько из них устанавливаются одновременно.
    /// @param flag проверяемый флаг.
    /// @param enabled Указывает, следует ли включать флаг(и).
    MAPI void FlagSetEnabled(RendererConfigFlags flag, bool enabled) {
        ptrRenderer->FlagSetEnabled(flag, enabled);
    }

    //////////////////////////////////////////////////////////////////////
    //                           RenderBuffer                           //
    //////////////////////////////////////////////////////////////////////

    /// @brief Создает и назначает буфер, специфичный для бэкэнда рендерера.
    /// @param buffer Указатель для создания внутреннего буфера.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferCreate(RenderBuffer& buffer);

    /// @brief Очищает указанный буфер. Внутренне сбрасывает список свободной памяти, если он используется.
    /// @param buffer ссылка на буфер, который необходимо освободить.
    /// @param ZeroMemory true, если память должна быть обнулена; в противном случае false. ПРИМЕЧАНИЕ: эта операция может быть дорогостоящей при больших объёмах памяти.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool RenderbufferClear(RenderBuffer& buffer, bool ZeroMemory);

    /// @brief Уничтожает указанный буфер.
    /// @param buffer Указатель на буфер, который необходимо уничтожить.
    MAPI void RenderBufferDestroy(RenderBuffer& buffer);

    /// @brief Связывает заданный буфер с указанным смещением.
    /// @param buffer Указатель на буфер для привязки.
    /// @param offset Смещение в байтах от начала буфера.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferBind(RenderBuffer& buffer, u64 offset) {
        return ptrRenderer->RenderBufferBind(buffer, offset);
    }

    /// @brief Отменяет привязку указанного буфера.
    /// @param buffer Указатель на буфер, который необходимо отменить.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferUnbind(RenderBuffer& buffer) {
        return ptrRenderer->RenderBufferUnbind(buffer);
    }

    /// @brief Отображает память из указанного буфера в предоставленном диапазоне в блок памяти и возвращает его. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отображения.
    /// @param offset Количество байтов от начала буфера для отображения.
    /// @param size Объем памяти в буфере для отображения.
    /// @returns Отображенный блок памяти. Освобожден и недействителен после отмены отображения.
    MAPI void* RenderBufferMapMemory(RenderBuffer& buffer, u64 offset, u64 size) {
        return ptrRenderer->RenderBufferMapMemory(buffer, offset, size);
    }
    
    /// @brief Отменяет отображение памяти из указанного буфера в предоставленном диапазоне в блок памяти. 
    /// Эта память должна считаться недействительной после отмены отображения.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для отмены отображения.
    /// @param size Объем памяти в буфере для отмены отображения.
    MAPI void RenderBufferUnmapMemory(RenderBuffer& buffer, u64 offset, u64 size) {
        ptrRenderer->RenderBufferUnmapMemory(buffer, offset, size);
    }

    /// @brief Очищает буферную память в указанном диапазоне. Должно быть сделано после записи.
    /// @param buffer Указатель на буфер для отмены отображения.
    /// @param offset Количество байтов от начала буфера для очистки.
    /// @param size Объем памяти в буфере для очистки.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferFlush(RenderBuffer& buffer, u64 offset, u64 size) {
        return ptrRenderer->RenderBufferFlush(buffer, offset, size);
    }

    /// @brief Считывает память из предоставленного буфера в указанном диапазоне в выходную переменную.
    /// @param buffer указатель на буфер для чтения.
    /// @param offset количество байтов от начала буфера для чтения.
    /// @param size объем памяти в буфере для чтения.
    /// @param OutMemory указатель на блок памяти для чтения. Должен быть соответствующего размера.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferRead(RenderBuffer& buffer, u64 offset, u64 size, void** OutMemory) {
        return ptrRenderer->RenderBufferRead(buffer, offset, size, OutMemory);
    }

    /// @brief Изменяет размер указанного буфера на NewTotalSize. NewTotalSize должен быть больше текущего размера буфера. Данные из старого внутреннего буфера копируются.
    ///
    /// @param buffer указатель на буфер, размер которого необходимо изменить.
    /// @param NewTotalSize новый размер в байтах. Должен быть больше текущего размера.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferResize(RenderBuffer& buffer, u64 NewTotalSize);

    /// @brief Загружает предоставленные данные в указанный диапазон указанного буфера.
    ///
    /// @param buffer указатель на буфер, в который нужно загрузить данные.
    /// @param offset смещение в байтах от начала буфера.
    /// @param size размер данных в байтах для загрузки.
    /// @param data данные для загрузки.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferLoadRange(RenderBuffer& buffer, u64 offset, u64 size, const void* data, bool IncludeInFrameWorkload) {
        return ptrRenderer->RenderBufferLoadRange(buffer, offset, size, data, IncludeInFrameWorkload);
    }

    /// @brief Копирует данные в указанном диапазоне из исходного буфера в целевой буфер.
    ///
    /// @param source указатель на исходный буфер, из которого копируются данные.
    /// @param SourceOffset смещение в байтах от начала исходного буфера.
    /// @param dest указатель на целевой буфер, в который копируются данные.
    /// @param DestOffset смещение в байтах от начала целевого буфера.
    /// @param size размер копируемых данных в байтах.
    /// @returns True в случае успеха; в противном случае false.
    MAPI bool RenderBufferCopyRange(RenderBuffer& source, u64 SourceOffset, RenderBuffer& dest, u64 DestOffset, u64 size, bool IncludeInFrameWorkload) {
        return ptrRenderer->RenderBufferCopyRange(source, SourceOffset, dest, DestOffset, size, IncludeInFrameWorkload);
    }

    /// @brief Пытается нарисовать содержимое предоставленного буфера с указанным смещением и количеством элементов. 
    /// Предназначено только для использования с буферами вершин и индексов.
    ///
    /// @param buffer указатель на буфер для рисования.
    /// @param offset смещение в байтах от начала буфера.
    /// @param ElementCount количество элементов для рисования.
    /// @param BindOnly только связывает буфер, но не вызывает draw.
    /// @return True в случае успеха; в противном случае false.
    MAPI bool RenderBufferDraw(RenderBuffer& buffer, u64 offset, u32 ElementCount, bool BindOnly) {
        return ptrRenderer->RenderBufferDraw(buffer, offset, ElementCount, BindOnly);
    }

    /// @brief 
    /// @return ссылку на текущую активную область просмотра
    MAPI Viewport* GetActiveViewport() { return ActiveViewport; }

    /// @brief Устанавливает текущую активную область просмотра.
    /// @param viewvport ccskrf на область просмотра, которую необходимо установить.
    MAPI void SetActiveViewport(Viewport* viewvport);

    /// @brief Ожидает, пока бэкенд рендерера полностью завершит всю работу, прежде чем вернуть управление. 
    /// ПРИМЕЧАНИЕ: Это влечет за собой значительные накладные расходы и требует ожидания, поэтому использовать данный механизм следует с осторожностью.
    MAPI void WaitForIdle() {
        ptrRenderer->WaitForIdle();
    }
};
