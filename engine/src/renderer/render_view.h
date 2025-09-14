#pragma once 
#include "containers/darray.hpp"
#include "core/event.h"
#include "math/matrix4d.h"

struct Renderpass;
struct Geometry;
struct Terrain;
struct FrameData;
struct RenderTargetAttachment;

struct GeometryRenderData 
{
    Matrix4D model;
    Geometry* geometry;
    u32 UniqueID;
};

template class DArray<GeometryRenderData>;

struct SkyboxPacketData {
    struct Skybox* sb;
    constexpr SkyboxPacketData(Skybox* sb) : sb(sb) {}
};

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
    struct RenderViewPacket {
        /// @brief Указатель на представление, с которым связан этот пакет.
        struct RenderView* view;
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

struct MAPI RenderView
{
// ЗАДАЧА: изменить архитектуру
    const char* name;               // Имя представления.
    u16 width;                      // Текущая ширина этого представления.
    u16 height;                     // Текущая высота этого представления.

    u8 RenderpassCount;             // Количество проходов рендеринга, используемых этим представлением.
    Renderpass* passes;             // Массив указателей на проходы рендеринга, используемые этим представлением.

    const char* CustomShaderName;   // Имя пользовательского шейдера, используемого этим представлением, если таковой имеется.
    
    void* data;             // Внутренние данные, специфичные для данного представления.

    constexpr RenderView() : name(), width(), height(), RenderpassCount(), passes(nullptr), CustomShaderName(nullptr), 
    OnRegistered(nullptr), Resize(nullptr), BuildPacket(nullptr), Render(nullptr), RegenerateAttachmentTarget(nullptr) 
    {}
    // constexpr RenderView(const char* name, u8 RenderpassCount, Renderpass* renderpass, const char* CustomShaderName, 
    //     bool (*OnRegistered)(RenderView*) = nullptr, void (*Resize)(RenderView*, u32, u32) = nullptr, 
    //     bool (*BuildPacket)(RenderView*, LinearAllocator&, void*, Packet&) = nullptr,
    //     bool (*Render)(RenderView*, const Packet&, u64, u64, const FrameData&) = nullptr, 
    //     bool (*RegenerateAttachmentTarget)(RenderView*, u32, RenderTargetAttachment*) = nullptr
    // );

    void DestroyPacket(RenderViewPacket& packet) {
        packet.geometries.Destroy();
        packet.TerrainGeometries.Destroy();
        packet.DebugGeometries.Destroy();
        MemorySystem::ZeroMem(&packet, sizeof(RenderViewPacket));
    }

    /// @brief Регистрирует данное представление
    /// @param self Указатель на представление для использования.
    /// @return true, если успешно иначе false
    bool (*OnRegistered)(RenderView* self);

    /// @brief Уничтожает данное представление
    /// @param self Указатель на представление для использования.
    void (*Destroy)(RenderView* self);

    /// @brief Измененяет размер владельца этого представления (например, окна).
    /// @param self Указатель на представление для использования.
    /// @param width новая ширина в пикселях.
    /// @param height новая высота в пикселях.
    void (*Resize)(RenderView* self, u32 width, u32 height);

    /// @brief Создает пакет представления рендеринга, используя предоставленное представление(view) и сетки(meshes).
    /// @param self Указатель на представление для использования.
    /// @param data данные свободной формы, используемые для создания пакета.
    /// @param OutPacket указатель для хранения сгенерированного пакета.
    /// @return true в случае успеха; в противном случае false.
    bool (*BuildPacket)(RenderView* self, struct LinearAllocator& FrameAllocator, void* data, RenderViewPacket& OutPacket);

    /// @brief Использует заданное представление и пакет для визуализации содержимого.
    /// @param self Указатель на представление для использования.
    /// @param packet указатель на пакет, данные которого должны быть визуализированы.
    /// @param FrameNumber текущий номер кадра визуализатора, обычно используемый для синхронизации данных.
    /// @param RenderTargetIndex текущий индекс цели визуализации для визуализаторов, которые используют несколько целей визуализации одновременно (например, Vulkan).
    /// @return true в случае успеха; в противном случае false.
    bool (*Render)(const RenderView* self, const RenderViewPacket& packet, u64 FrameNumber, u64 RenderTargetIndex, const FrameData& rFrameData);

    /// @brief Регенерирует ресурсы для указанного вложения по указанному индексу прохода.
    /// @param self Указатель на представление для использования.
    /// @param PassIndex Индекс прохода рендеринга для генерации.
    /// @param attachment Указатель на вложение, ресурсы которого должны быть регенерированы.
    /// @return True в случае успеха; в противном случае false.
    bool (*RegenerateAttachmentTarget)(RenderView* self, u32 PassIndex, RenderTargetAttachment* attachment);
};

MAPI bool RenderViewOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);

