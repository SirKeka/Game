#pragma once

#include "math/matrix4d.h"
#include "math/vertex.h"
#include "renderbuffer.h"
#include "resources/mesh.h"

struct Text;
struct UiGeometry;

/*размер данной струтуры для карт Nvidia должен быть равен 256 байт*/
struct VulkanMaterialShaderGlobalUniformObject
{
    Matrix4D projection;    // 64 байта
    Matrix4D view;          // 64 байта
    Matrix4D mReserved0;    // 64 байта, зарезервированные для будущего использования
    Matrix4D mReserved1;    // 64 байта, зарезервированные для будущего использования
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
    Matrix4D projection;    // 64 байт
    Matrix4D view;          // 64 байт
    Matrix4D mReserved0;    // 64 байт, зарезервировано для будущего использования.
    Matrix4D mReserved1;    // 64 байт, зарезервировано для будущего использования.
};

/// @brief Объект универсального буфера экземпляра материала пользовательского интерфейса, специфичный для Vulkan, для шейдера пользовательского интерфейса.
struct VulkanUI_ShaderInstanceUniformObject {
    FVec4 DiffuseColor; // 16 байт
    FVec4 vReserved0;   // 16 байт, зарезервировано для будущего использования.
    FVec4 vReserved1;   // 16 байт, зарезервировано для будущего использования.
    FVec4 vReserved2;   // 16 байт, зарезервировано для будущего использования.
};

struct UiPacketData {
    Mesh::PacketData MeshData;
};

namespace Render
{
    enum DebugViewMode : u8 {
        Default = 0,
        Lighting,
        Normals,
        Cascades,
        Wireframe
    };
} // namespace Render

namespace RenderingConfigFlagBits {
    enum {
        VsyncEnabledBit = 0x1,  // Указывает, что vsync следует включить.
        PowerSavingBit = 0x2,   // Настраивает рендерер таким образом, чтобы экономить электроэнергию, где это возможно.
        EnableValidation = 0x4, // Включает расширенную проверку в бэкенде отрисовки (при наличии поддержки).
    };
}

using RendererConfigFlags = u32;

/// @brief Общая конфигурация для рендерера.
struct RenderingConfig {
    const char* ApplicationName;    // Имя приложения.
    RendererConfigFlags flags;      // Различные флаги конфигурации для настройки рендерера.
};

enum class RendererStencilOp {
    /// @brief Сохраняет текущее значение.
    Keep = 0,
    /// @brief Устанавливает значение буфера трафарета равным 0.
    Zero = 1,
    /// @brief Устанавливает значение буфера трафарета равным _ref_, как указано в функции.
    Replace = 2,
    /// @brief Увеличивает текущее значение буфера трафарета. Устанавливает ограничение на максимальное представимое беззнаковое значение.
    IncrementAndClamp = 3,
    /// @brief Уменьшает текущее значение буфера трафарета. Устанавливает ограничение на 0.
    DecrementAndClamp = 4,
    /// @brief Побитовое инвертирование текущего значения буфера трафарета.
    Invert = 5,
    /// @brief Увеличивает текущее значение буфера трафарета. Оборачивает значение буфера трафарета до нуля при увеличении максимального представимого беззнакового значения.
    IncrementAandWrap = 6,
    /// @brief Уменьшает текущее значение буфера трафарета. Оборачивает значение буфера трафарета до максимального представимого беззнакового значения при уменьшении значения буфера трафарета до нуля.
    DecrementAndWrap = 7
};

enum class RendererCompareOp {
    /// @brief Указывает, что сравнение всегда возвращает false.
   Never = 0,
    /// @brief Указывает, что сравнение возвращает reference < test.
   Less = 1,
    /// @brief Указывает, что сравнение возвращает reference = test.
   Equal = 2,
    /// @brief Указывает, что сравнение возвращает reference <= test.
   LessOrEqual = 3,
    /// @brief Указывает, что сравнение возвращает reference > test.
   Greater = 4,
    /// @brief Указывает, что сравнение возвращает reference != test.
   NotEqual = 5,
    /// @brief Указывает, что сравнение возвращает reference >= test.
   GreaterOrEqual = 6,
    /// @brief Указывает, что сравнение всегда возвращает true.
   Always = 7
};

/// @brief Порядок обхода вершин, используемый для определения передней грани треугольника.
enum class RendererWinding 
{
    /// @brief Обход вершин против часовой стрелки.
   CounterClockwise = 0,
    /// @brief Обход вершин против часовой стрелки.
    Clockwise = 1
};

enum class RendererProjectionMatrixType {
    Perspective = 0x0,
    /// @brief Ортографическая матрица, центрированная на нуле в левом верхнем углу.
    Orthographic = 0x1,
    /// @brief Ортографическая матрица, центрированная на ширине/высоте, а не на нулевом уровне. Использует поле зрения (FOV) в качестве «масштаба».
    OrthographicCentered = 0x2
};

/// @param Matrix4D model
/// @param Material* material 
/// @param Identifier UniqueID 
/// @param bool WindingInverted
/// @param FVec4 DiffuseColour
/// @param u32 VertexCount
/// @param u32 VertexElementSize
/// @param u64 VertexBufferOffset
/// @param u32 IndexCount
/// @param u32 IndexElementSize
/// @param u64 IndexBufferOffset

/// @param Matrix4D model
/// @param Geometry* geometry
/// @param u64 UniqueID
/// @param void* UiData
/// @param FVec4 DiffuseColour
/// @param bool WindingInverted;
struct GeometryRenderData 
{
    Matrix4D model;
    // ЗАДАЧА: Вместо этого сохраните идентификатор/дескриптор.
    struct Material* material; // struct Geometry* geometry{};
    u64 UniqueID;
    /// @brief Количество вершин.
    u32 VertexCount;
    /// @brief Размер каждой вершины.
    u32 VertexElementSize;
    /// @brief Смещение от начала буфера вершин.
    u64 VertexBufferOffset;

    /// @brief Количество индексов.
    u32 IndexCount;
    /// @brief Размер каждого индекса.
    u32 IndexElementSize;
    /// @brief Смещение от начала буфера индексов.
    u64 IndexBufferOffset;
    FVec4 DiffuseColour;
    bool WindingInverted;

    MAPI void CopyData(const struct Geometry* geometry);
};
