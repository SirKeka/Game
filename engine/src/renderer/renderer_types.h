#pragma once

//#include "renderpass.hpp"
#include "math/matrix4d.h"
#include "math/vertex.h"
#include "resources/shader.h"
#include "render_view.h"
#include "renderbuffer.h"
#include "resources/mesh.h"
#include "resources/ui_text.h"
#include "renderer_types.h"

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

/// @brief Структура, которая генерируется приложением и отправляется один раз рендереру для рендеринга заданного кадра. 
/// Состоит из любых требуемых данных, таких как дельта-время и коллекция представлений для рендеринга.
struct RenderPacket
{
    u16 ViewCount;              // Количество представлений, которые нужно отобразить. 
    RenderView::Packet* views;  // Массив представлений, которые нужно отобразить.
    void Destroy() {
        for (u16 i = 0; i < ViewCount; i++) {
            views[i].geometries.Destroy();
            views[i].TerrainGeometries.Destroy();
            views[i].DebugGeometries.Destroy();
        }
    }
};

struct UiPacketData {
    Mesh::PacketData MeshData;
    // ЗАДАЧА: временно
    u32 TextCount;
    Text** texts;
    // constexpr UiPacketData(Mesh::PacketData MeshData, u32 TextCount, Text** texts) : MeshData(MeshData), TextCount(TextCount), texts(texts) {}
};

namespace Render
{
    enum DebugViewMode : u32 {
        Default = 0,
        Lighting = 1,
        Normals = 2
    };
} // namespace Render

enum RenderingConfigFlagBits {
    VsyncEnabledBit = 0x1,  // Указывает, что vsync следует включить.
    PowerSavingBit = 0x2,   // Настраивает рендерер таким образом, чтобы экономить электроэнергию, где это возможно.
};

using RendererConfigFlags = u32;

/// @brief Общая конфигурация для рендерера.
struct RenderingConfig {
    const char* ApplicationName;    // Имя приложения.
    RendererConfigFlags flags;      // Различные флаги конфигурации для настройки рендерера.
};
