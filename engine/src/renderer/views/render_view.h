#pragma once 
#include "containers/darray.hpp"
#include "core/event.hpp"
#include "math/matrix4d.h"

struct Renderpass;
struct GeometryID;
struct Terrain;
struct FrameData;

struct GeometryRenderData 
{
    Matrix4D model;
    GeometryID* geometry;
    u32 UniqueID;
};

template class DArray<GeometryRenderData>;

struct SkyboxPacketData {
    struct Skybox* sb;
    constexpr SkyboxPacketData(Skybox* sb) : sb(sb) {}
};

class MAPI RenderView
{
public:
    /// @brief Известные типы представления рендеринга, имеющие связанную с ними логику.
enum KnownType {
    World  = 0x01, // Представление, которое рендерит только объекты с *никакой* прозрачностью.
    UI     = 0x02, // Представление, которое рендерит только объекты пользовательского интерфейса.
    Skybox = 0x03, // Вид, отображающий только объекты скайбокса.
    Pick   = 0x04  // Представление, которое отображает только объекты пользовательского интерфейса и мира, которые можно выбрать.
};

/// @brief Известные источники матрицы представления.
enum ViewMatrixSource {
    SceneCamera = 0x01,
    UiCamera    = 0x02,
    LightCamera = 0x03,
};

/// @brief Известные источники матрицы проекции.
enum ProjectionMatrixSource {
    DefaultPerspective  = 0x01,
    DefaultOrthographic = 0x02,
};

/// @brief Конфигурация представления рендеринга. Используется как цель сериализации.
struct Config {
    const char* name;               // Имя представления.
    const char* CustomShaderName;   // Имя пользовательского шейдера, который будет использоваться вместо шейдера по умолчанию для представления. Должен быть 0, если не используется.
    u16 width;                      // Ширина представления. Установите на 0 для ширины 100%.
    u16 height;                     // Высота представления. Установите на 0 для высоты 100%.
    KnownType type;                 // Известный тип представления. Используется для связи с логикой представления.
    ViewMatrixSource VMS;           // Источник матрицы представления.
    ProjectionMatrixSource PMS;     // Источник матрицы проекции.
    u8 PassCount;                   // Количество проходов рендеринга, используемых в этом представлении.
    struct RenderpassConfig* passes;// Конфигурация проходов рендеринга, используемых в этом представлении.
};

struct Packet;
// ЗАДАЧА: изменить архитектуру
protected:
    friend class RenderViewSystem;
    friend struct SimpleScene;
    u16 id;                         // Уникальный идентификатор этого представления.
    MString name;                   // Имя представления.
    u16 width;                      // Текущая ширина этого представления.
    u16 height;                     // Текущая высота этого представления.
    KnownType type;                 // Известный тип этого представления.

    u8 RenderpassCount;             //Количество проходов рендеринга, используемых этим представлением.
    Renderpass* passes;             //Массив указателей на проходы рендеринга, используемые этим представлением.

    const char* CustomShaderName;   // Имя пользовательского шейдера, используемого этим представлением, если таковой имеется.
public:
    constexpr RenderView()
    : id(INVALID::U16ID), name(), width(), height(), type(), RenderpassCount(), passes(nullptr), CustomShaderName(nullptr) {}
    RenderView(u16 id, const Config &config);
    constexpr RenderView(u16 id, MString&& name, u16 width, u16 height, KnownType type, u8 RenderpassCount, const char* CustomShaderName);
    virtual ~RenderView();

    void DestroyPacket(Packet& packet) {
        packet.geometries.Destroy();
        packet.TerrainGeometries.Destroy();
        packet.DebugGeometries.Destroy();
        MemorySystem::ZeroMem(&packet, sizeof(Packet));
    }

    /// @brief Измененяет размер владельца этого представления (например, окна).
    /// @param width новая ширина в пикселях.
    /// @param height новая высота в пикселях.
    virtual void Resize(u32 width, u32 height) {}

    /// @brief Создает пакет представления рендеринга, используя предоставленное представление(view) и сетки(meshes).
    /// @param data данные свободной формы, используемые для создания пакета.
    /// @param OutPacket указатель для хранения сгенерированного пакета.
    /// @return true в случае успеха; в противном случае false.
    virtual bool BuildPacket(class LinearAllocator& FrameAllocator, void* data, Packet& OutPacket) { return false; }

    /// @brief Использует заданное представление и пакет для визуализации содержимого.
    /// @param packet указатель на пакет, данные которого должны быть визуализированы.
    /// @param FrameNumber текущий номер кадра визуализатора, обычно используемый для синхронизации данных.
    /// @param RenderTargetIndex текущий индекс цели визуализации для визуализаторов, которые используют несколько целей визуализации одновременно (например, Vulkan).
    /// @return true в случае успеха; в противном случае false.
    virtual bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData) { return false; }

    /// @brief Регенерирует ресурсы для указанного вложения по указанному индексу прохода.
    /// @param self Указатель на представление для использования.
    /// @param PassIndex Индекс прохода рендеринга для генерации.
    /// @param attachment Указатель на вложение, ресурсы которого должны быть регенерированы.
    /// @return True в случае успеха; в противном случае false.
    virtual bool RegenerateAttachmentTarget(u32 PassIndex = 0, struct RenderTargetAttachment* attachment = nullptr);
 
    /// @brief Пакет для представления рендеринга, созданный им и содержащий данные о том, что должно быть визуализировано.
    /// @param RenderView*_view Указатель на представление, с которым связан этот пакет.
    /// @param Matrix4D_ViewMatrix Текущая матрица представления.
    /// @param Matrix4D_ProjectionMatrix Текущая матрица проекции.
    /// @param FVec3_ViewPosition Текущая позиция представления, если применимо.
    /// @param FVec4_AmbientColour Текущий окружающий цвет сцены, если применимо.
    /// @param u32_GeometryCount Количество геометрий для рисования.
    /// @param GeometryRenderData*_geometries Геометрии для рисования.
    /// @param const_char*_CustomShaderName Имя используемого пользовательского шейдера, если применимо. В противном случае 0.
    /// @param void*_ExtendedData Содержит указатель на данные свободной формы, обычно понимаемые как объектом, так и потребляющим представлением.
    struct Packet {
        /// @brief Указатель на представление, с которым связан этот пакет.
        RenderView* view;
        /// @brief Текущая матрица вида.
        Matrix4D ViewMatrix;
        /// @brief Текущая проекционная матрица.
        Matrix4D ProjectionMatrix;
        /// @brief Текущая позиция просмотра, если применимо.
        FVec3 ViewPosition;
        /// @brief Текущий окружающий цвет сцены, если применимо.
        FVec4 AmbientColour;
        /// @brief Количество геометрических фигур, которые необходимо нарисовать.
        // u32 GeometryCount;
        /// @brief Геометрии, которые необходимо нарисовать.
        DArray<GeometryRenderData> geometries;
        /// @brief Геометрия местности, которую необходимо нарисовать.
        DArray<GeometryRenderData> TerrainGeometries;
        /// @brief Отладочные геометрии, которые необходимо нарисовать.
        DArray<GeometryRenderData> DebugGeometries;
        /// @brief 
        Terrain** terrains;
        /// @brief Имя пользовательского шейдера для использования, если применимо. В противном случае nullptr.
        const char* CustomShaderName;
        /// @brief Содержит указатель на данные произвольной формы, обычно понятные как объекту, так и потребляющему представлению.
        void* ExtendedData;
    };
};

bool RenderViewOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);

