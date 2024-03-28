#pragma once
#include "containers/mstring.hpp"

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
public:
    Geometry();
    Geometry(u32 id, u32 InternalID, u32 generation, MString name, u32 VertexCount, u32 VertexSize, u32 VertexBufferOffset, u32 IndexCount, u32 IndexSize, u32 IndexBufferOffset, class Material* material);
    Geometry(const Geometry& g);
    Geometry(Geometry&& g);

    /*bool Create(u32 VertexCount, const Vertex3D& vertices, u32 IndexCount, u32* indices);
    void Destroy();*/
};