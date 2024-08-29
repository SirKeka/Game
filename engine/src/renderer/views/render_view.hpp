#pragma once 
#include "math/matrix4d.hpp"
#include "containers/darray.hpp"

struct Renderpass;
struct SkyboxPacketData {
    class Skybox* sb;
    constexpr SkyboxPacketData() : sb(nullptr) {}
    constexpr SkyboxPacketData(Skybox* sb) : sb(sb) {}
};

class RenderView
{
public:
    /// @brief Известные типы представления рендеринга, имеющие связанную с ними логику.
enum KnownType {
    KnownTypeWorld  = 0x01, // Представление, которое рендерит только объекты с *никакой* прозрачностью.
    KnownTypeUI     = 0x02, // Представление, которое рендерит только объекты пользовательского интерфейса.
    KnownTypeSkybox = 0x03  // Вид, отображающий только объекты скайбокса.
};

/// @brief Известные источники матрицы представления.
enum ViewMatrixSource {
    ViewMatrixSourceSceneCamera = 0x01,
    ViewMatrixSourceUiCamera    = 0x02,
    ViewMatrixSourceLightCamera = 0x03,
};

/// @brief Известные источники матрицы проекции.
enum ProjectionMatrixSource {
    ProjectionMatrixSourceDefaultPerspective  = 0x01,
    ProjectionMatrixSourceDefaultOrthographic = 0x02,
};

/// @brief конфигурация для рендерпасса, связанного с представлением
struct PassConfig {
    const char* name;
    constexpr PassConfig(const char* name) : name(name) {}
};

/// @brief Конфигурация представления рендеринга. Используется как цель сериализации.
struct Config {
    MString name;                   // Имя представления.

    const char* CustomShaderName;   // Имя пользовательского шейдера, который будет использоваться вместо шейдера по умолчанию для представления. Должен быть 0, если не используется.
    u16 width;                      // Ширина представления. Установите на 0 для ширины 100%.
    u16 height;                     // Высота представления. Установите на 0 для высоты 100%.
    KnownType type;                 // Известный тип представления. Используется для связи с логикой представления.
    ViewMatrixSource VMS;           // Источник матрицы представления.
    ProjectionMatrixSource PMS;     // Источник матрицы проекции.
    u8 PassCount;                   // Количество проходов рендеринга, используемых в этом представлении.
    PassConfig* passes;             // Конфигурация проходов рендеринга, используемых в этом представлении.

    constexpr Config(const char* name, u16 width, u16 height, KnownType type, ViewMatrixSource VMS, u8 PassCount, PassConfig* passes)
    : name(name), CustomShaderName(nullptr), width(width), height(height), type(type), VMS(VMS), PMS(), PassCount(PassCount), passes(passes) {}
};

struct Packet;
// ЗАДАЧА: изменить архитектуру
protected:
    friend class RenderViewSystem;
    u16 id;                         // Уникальный идентификатор этого представления.
    MString name;                   // Имя представления.
    u16 width;                      // Текущая ширина этого представления.
    u16 height;                     // Текущая высота этого представления.
    KnownType type;                 // Известный тип этого представления.

    u8 RenderpassCount;             //Количество проходов рендеринга, используемых этим представлением.
    Renderpass** passes;            //Массив указателей на проходы рендеринга, используемые этим представлением.

    const char* CustomShaderName;   // Имя пользовательского шейдера, используемого этим представлением, если таковой имеется.
    //void* data;                     // Внутренние данные, специфичные для представления, для этого представления.
public:
    constexpr RenderView()
    : id(INVALID::U16ID), name(), width(), height(), type(), RenderpassCount(), passes(nullptr), CustomShaderName(nullptr) {}
    constexpr RenderView(u16 id, MString& name, KnownType type, u8 RenderpassCount, const char* CustomShaderName) 
    : id(id), name(std::move(name)), width(), height(), type(type), RenderpassCount(RenderpassCount), passes(MMemory::TAllocate<Renderpass*>(Memory::Renderer, RenderpassCount)), CustomShaderName(CustomShaderName) {}
    constexpr RenderView(u16 id, MString& name, u16 width, u16 height, KnownType type, u8 RenderpassCount, const char* CustomShaderName)
    : id(id), name(std::move(name)), width(width), height(height), type(type), RenderpassCount(RenderpassCount), passes(MMemory::TAllocate<Renderpass*>(Memory::Renderer, RenderpassCount)), CustomShaderName(CustomShaderName) {}
    virtual ~RenderView() { MMemory::Free(passes, sizeof(Renderpass*) * RenderpassCount, Memory::Renderer); }

    /// @brief Измененяет размер владельца этого представления (например, окна).
    /// @param width новая ширина в пикселях.
    /// @param height новая высота в пикселях.
    virtual void Resize(u32 width, u32 height) = 0;
    /// @brief Создает пакет представления рендеринга, используя предоставленное представление(view) и сетки(meshes).
    /// @param data данные свободной формы, используемые для создания пакета.
    /// @param OutPacket указатель для хранения сгенерированного пакета.
    /// @return true в случае успеха; в противном случае false.
    virtual bool BuildPacket(void* data, Packet& OutPacket) const = 0;
    /// @brief Использует заданное представление и пакет для визуализации содержимого.
    /// @param packet указатель на пакет, данные которого должны быть визуализированы.
    /// @param FrameNumber текущий номер кадра визуализатора, обычно используемый для синхронизации данных.
    /// @param RenderTargetIndex текущий индекс цели визуализации для визуализаторов, которые используют несколько целей визуализации одновременно (например, Vulkan).
    /// @return true в случае успеха; в противном случае false.
    virtual bool Render(const Packet& packet, u64 FrameNumber, u64 RenderTargetIndex) const = 0;
 
    /// @brief Пакет для представления рендеринга, созданный им и содержащий данные о том, что должно быть визуализировано.
    struct Packet {
        const RenderView* view;                         // Постоянный указатель на представление, с которым связан этот пакет.
        Matrix4D ViewMatrix;                            // Текущая матрица представления.
        Matrix4D ProjectionMatrix;                      // Текущая матрица проекции.
        FVec3 ViewPosition;                             // Текущая позиция представления, если применимо.
        FVec4 AmbientColour;                            // Текущий окружающий цвет сцены, если применимо.
        u32 GeometryCount;                              // Количество геометрий для рисования.
        DArray<struct GeometryRenderData> geometries;   // Геометрии для рисования.
        const char* CustomShaderName;                   // Имя используемого пользовательского шейдера, если применимо. В противном случае 0.
        void* ExtendedData;                             // Содержит указатель на данные свободной формы, обычно понимаемые как объектом, так и потребляющим представлением.
    };
};
