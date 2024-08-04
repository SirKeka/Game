#pragma once

#include "renderpass.hpp"
#include "math/matrix4d.hpp"
#include "math/vertex.hpp"
#include "resources/shader.hpp"

struct StaticMeshData;
class Texture;
class Renderer;

namespace RendererDebugViewMode {
    enum RendererDebugViewMode : u32 {
        Default = 0,
        Lighting = 1,
        Normals = 2
};
}

enum class ERendererType 
{
    VULKAN,
    OPENGL,
    DIRECTX
};

/*размер данной струтуры для карт Nvidia должен быть равен 256 байт*/
struct VulkanMaterialShaderGlobalUniformObject
{
    Matrix4D projection;        // 64 байта
    Matrix4D view;              // 64 байта
    Matrix4D mReserved0;        // 64 байта, зарезервированные для будущего использования
    Matrix4D mReserved1;        // 64 байта, зарезервированные для будущего использования
};

struct VulkanMaterialShaderInstanceUniformObject 
{
    FVec4 DiffuseColor; // 16 байт
    FVec4 vReserved0;   // 16 байт, зарезервировано для будущего использования.
    FVec4 vReserved1;   // 16 байт, зарезервировано для будущего использования.
    FVec4 vReserved2;   // 16 байт, зарезервировано для будущего использования.
};

/// @brief 
struct VulkanUI_ShaderGlobalUniformObject {
    Matrix4D projection;        // 64 bytes
    Matrix4D view;              // 64 bytes
    Matrix4D mReserved0;        // 64 bytes, зарезервировано для будущего использования.
    Matrix4D mReserved1;        // 64 bytes, зарезервировано для будущего использования.
};

/// @brief Объект универсального буфера экземпляра материала пользовательского интерфейса, специфичный для Vulkan, для шейдера пользовательского интерфейса.
struct VulkanUI_ShaderInstanceUniformObject {
    FVec4 DiffuseColor; // 16 bytes
    FVec4 vReserved0;   // 16 bytes, зарезервировано для будущего использования.
    FVec4 vReserved1;   // 16 bytes, зарезервировано для будущего использования.
    FVec4 vReserved2;   // 16 bytes, зарезервировано для будущего использования.
};

struct GeometryRenderData 
{
    Matrix4D model;
    struct GeometryID* gid;

    constexpr GeometryRenderData() : model(), gid(nullptr) {}
    constexpr GeometryRenderData(const Matrix4D& model, struct GeometryID* gid) : model(model), gid(gid) {}
    //constexpr GeometryRenderData(GeometryRenderData&& grd) : model(grd.model), gid() {}
};

/* f32 DeltaTime
 * u32 GeometryCount
 * struct GeometryRenderData* geometries
 * u32 UI_GeometryCount
 * struct GeometryRenderData* UI_Geometries*/
struct RenderPacket
{
    f64 DeltaTime;
    u32 GeometryCount;
    DArray<GeometryRenderData> geometries;
    u32 UI_GeometryCount;
    DArray<GeometryRenderData> UI_Geometries;
    constexpr RenderPacket() : DeltaTime(), GeometryCount(), geometries(), UI_GeometryCount(), UI_Geometries() {}
    constexpr RenderPacket(f64 DeltaTime, u32 GeometryCount, GeometryRenderData geometries, u32 UI_GeometryCount, GeometryRenderData UI_Geometries)
    : DeltaTime(DeltaTime), 
    GeometryCount(GeometryCount), 
    geometries(), 
    UI_GeometryCount(UI_GeometryCount), 
    UI_Geometries() {
        this->geometries.PushBack(std::move(geometries));
        this->UI_Geometries.PushBack(std::move(UI_Geometries));
    }
};

/// @brief Общая конфигурация для рендерера.
struct RendererConfig {
    const char* ApplicationName;            // Имя приложения.
    u16 RenderpassCount;                    // Количество указателей на проходы рендеринга.
    struct RenderpassConfig* PassConfigs;   // Массив конфигураций для проходов рендеринга. Будет инициализировано на бэкэнде автоматически.
    // Обратный вызов, который будет выполнен, когда бэкэнд потребует обновления/повторной генерации целей рендеринга.
    class PFN_Method {
    public:
        using PtrCallbackMethod = void(Renderer::*)();

        constexpr PFN_Method(Renderer* ptrCallbackClass, PtrCallbackMethod CallbackMethod)
        : ptrCallbackClass(ptrCallbackClass), CallbackMethod(CallbackMethod) {}
        void Run() const {
            (ptrCallbackClass->*CallbackMethod)();
        }
        operator bool() const { return (ptrCallbackClass != nullptr && CallbackMethod != nullptr); }
    private:
        Renderer* ptrCallbackClass{nullptr};
        PtrCallbackMethod CallbackMethod{nullptr};
    } OnRendertargetRefreshRequired;
    // void (Renderer::*OnRendertargetRefreshRequired)();
    constexpr RendererConfig(const char* ApplicationName, u16 RenderpassCount, struct RenderpassConfig* PassConfigs, PFN_Method OnRendertargetRefreshRequired)
    : ApplicationName(ApplicationName), RenderpassCount(RenderpassCount), PassConfigs(PassConfigs), OnRendertargetRefreshRequired(OnRendertargetRefreshRequired) {}
};

class RendererType
{
public:
    u64 FrameNumber;

    // Указатель на текстуру по умолчанию.
    //class Texture* DefaultDiffuse;
public:
    virtual ~RendererType() = default;

    // virtual bool Initialize(class MWindow* window, const char* ApplicationName) = 0;
    virtual void ShutDown() = 0;
    virtual void Resized(u16 width, u16 height) = 0;
    virtual bool BeginFrame(f32 Deltatime) = 0;
    virtual bool EndFrame(f32 DeltaTime) = 0;
    /// @brief Начинает проход рендеринга с указанной целью.
    /// @param pass указатель на проход рендеринга для начала.
    /// @param target указатель на цель рендеринга для использования.
    /// @return тrue в случае успеха иначе false.
    virtual bool BeginRenderpass(Renderpass* pass, RenderTarget& target) = 0;
    /// @brief Завершает проход рендеринга с указанным идентификатором.
    /// @param pass указатель на проход рендеринга для завершения.
    /// @return тrue в случае успеха иначе false.
    virtual bool EndRenderpass(Renderpass* pass) = 0;
    /// @brief Получает указатель на renderpass, используя предоставленное имя.
    /// @param name имя renderpass.
    /// @return Указатель на renderpass, если найден; в противном случае nullptr.
    virtual Renderpass* GetRenderpass(const MString& name) = 0;
    /// @brief Рисует заданную геометрию. Должен вызываться только внутри прохода рендеринга, внутри кадра.
    /// @param data данные рендеринга геометрии, которая должна быть нарисована.
    virtual void DrawGeometry(const GeometryRenderData& data) = 0;

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
    /// @brief Выгружает данные текстуры из графического процессора.
    /// @param texture указатель на текстуру которую нужно выгрузить.
    virtual void Unload(Texture* texture) = 0;

    virtual bool Load(struct GeometryID* gid, u32 VertexSize, u32 VertexCount, const void* vertices, u32 IndexSize, u32 IndexCount, const void* indices) = 0;
    virtual void Unload(struct GeometryID* gid) = 0;
    // Методы относящиеся к шейдерам---------------------------------------------------------------------------------------------------------------------------------------------

    /// @brief Создает внутренние ресурсы шейдера, используя предоставленные параметры.
    /// @param shader указатель на шейдер.
    /// @param renderpass указатель на проход рендеринга, который будет связан с шейдером.
    /// @param StageCount общее количество этапов.
    /// @param StageFilenames массив имен файлов этапов шейдера, которые будут загружены. Должно соответствовать массиву этапов.
    /// @param stages массив этапов шейдера(ShaderStage), указывающий, какие этапы рендеринга (вершина, фрагмент и т. д.) используются в этом шейдере.
    /// @return true в случае успеха, иначе false.
    virtual bool Load(Shader* shader, Renderpass* renderpass, u8 StageCount, const DArray<MString>& StageFilenames, const ShaderStage* stages) = 0;
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
    virtual bool ShaderApplyGlobals(Shader* shader) = 0;
    /// @brief Применяет данные для текущего привязанного экземпляра.---------------------------------------------------------------------------------
    /// @param shader указатель на шейдер, глобальные значения которого должны быть связаны.
    /// @param NeedsUpdate указывает на то что нужно обновить униформу шейдера или только привязать.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderApplyInstance(Shader* shader, bool NeedsUpdate) = 0;
    /// @brief Получает внутренние ресурсы уровня экземпляра и предоставляет идентификатор экземпляра.------------------------------------------------
    /// @param shader указатель на шейдер, к которому нужно применить данные экземпляра.
    /// @param OutInstanceID ссылка для хранения нового идентификатора экземпляра.
    /// @return true в случае успеха, иначе false.
    virtual bool ShaderAcquireInstanceResources(Shader* shader, struct TextureMap** maps, u32& OutInstanceID) = 0;
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
    virtual bool SetUniform(Shader* shader, struct ShaderUniform* uniform, const void* value) = 0;
    /// @brief Создает новую цель рендеринга, используя предоставленные данные.
    /// @param AttachmentCount количество вложений (указателей текстур).
    /// @param attachments массив вложений (указателей текстур).
    /// @param pass указатель на проход рендеринга, с которым связана цель рендеринга.
    /// @param width ширина цели рендеринга в пикселях.
    /// @param height высота цели рендеринга в пикселях.
    /// @param OutTarget указатель для хранения новой созданной цели рендеринга.
    virtual void RenderTargetCreate(u8 AttachmentCount, Texture** attachments, Renderpass* pass, u32 width, u32 height, RenderTarget& OutTarget) = 0;
    /// @brief Уничтожает предоставленную цель рендеринга.
    /// @param target указатель на цель рендеринга, которую нужно уничтожить.
    /// @param FreeInternalMemory указывает, следует ли освободить внутреннюю память.
    virtual void RenderTargetDestroy(RenderTarget& target, bool FreeInternalMemory) = 0;
    /// @brief Создает новый проход рендеринга.
    /// @param OutRenderpass указатель на общий проход рендеринга.
    /// @param depth величина очистки глубины.
    /// @param stencil значение очистки трафарета.
    /// @param HasPrevPass указывает, есть ли предыдущий проход рендеринга.
    /// @param HasNextPass указывает, есть ли следующий проход рендеринга.
    virtual void RenderpassCreate(Renderpass* OutRenderpass, f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass) = 0;
    /// @brief Уничтожает указанный renderpass.
    /// @param OutRenderpass указатель на renderpass, который необходимо уничтожить.
    virtual void RenderpassDestroy(Renderpass* OutRenderpass) = 0;
    /// @brief Пытается получить цель визуализации окна по указанному индексу.
    /// @param index индекс вложения для получения. Должен быть в пределах диапазона количества целей визуализации окна.
    /// @return указатель на вложение текстуры в случае успеха; в противном случае 0.
    virtual Texture* WindowAttachmentGet(u8 index) = 0;
    /// @return Возвращает указатель на основную цель текстуры глубины.
    virtual Texture* DepthAttachmentGet() = 0;
    /// @return Возвращает текущий индекс прикрепления окна.
    virtual u8 WindowAttachmentIndexGet() = 0;

    };
