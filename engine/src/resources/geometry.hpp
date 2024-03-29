#pragma once
#include "containers/mstring.hpp"
#include "math/vertex3D.hpp"

#define GEOMETRY_NAME_MAX_LENGTH 256

// Максимальное количество одновременно загружаемых геометрий
// TODO: сделать настраиваемым
#define VULKAN_MAX_GEOMETRY_COUNT 4096

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
class Geometry {
private:
    u32 id;
    u32 InternalID;
    u32 generation;
    MString name;

    u32 VertexCount;
    u32 VertexSize;
    u32 VertexBufferOffset;
    u32 IndexCount;
    u32 IndexSize;
    u32 IndexBufferOffset;

    class Material* material;

    friend class GeometrySystem;
    friend class VulkanAPI;
public:
    Geometry();
    Geometry(u32 id, u32 InternalID, u32 generation, MString name, u32 VertexCount, u32 VertexSize, u32 VertexBufferOffset, u32 IndexCount, u32 IndexSize, u32 IndexBufferOffset, class Material* material);
    Geometry(const Geometry& g);
    Geometry(Geometry&& g);

    Geometry& operator= (const Geometry& g);
    Geometry& operator= (const Geometry* g);

    void SetVertexData(u32 VertexCount, u32 VertexSize, u32 VertexBufferOffset);
    void SetIndexData(u32 IndexCount, u32 IndexSize, u32 IndexBufferOffset);

    /// @brief Создает геометрию и отправляет ее в систему
    /// @param VertexCount количество вершин геометрии
    /// @param vertices параметры вершин геометрии
    /// @param IndexCount количество индексов вершин
    /// @param indices 
    /// @return возвращает true при успехе, иначе false
    bool SendToRender(class RendererType *rType, u32 VertexCount, const Vertex3D* vertices, u32 IndexCount, const u32* indices);
    void Destroy();
};