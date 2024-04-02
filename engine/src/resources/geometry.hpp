#pragma once
#include "containers/mstring.hpp"
#include "math/vertex3D.hpp"
#include "systems/material_system.hpp"

#define GEOMETRY_NAME_MAX_LENGTH 256

// Максимальное количество одновременно загружаемых геометрий
// TODO: сделать настраиваемым
#define VULKAN_MAX_GEOMETRY_COUNT 4096

struct GeometryID {
    u32 id;
    u32 InternalID;
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    class Material* material;
};

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
class Geometry {
private:
    u32 id;
    u32 generation;
    //char name[GEOMETRY_NAME_MAX_LENGTH];

    u32 VertexCount;
    u32 VertexSize;
    u32 VertexBufferOffset;
    u32 IndexCount;
    u32 IndexSize;
    u32 IndexBufferOffset;

    //friend class GeometrySystem;
    friend class VulkanAPI;
public:
    Geometry();
    Geometry(u32 VertexCount, u32 VertexBufferOffset, u32 IndexCount, u32 IndexBufferOffset);
    Geometry(const Geometry& g);
    Geometry(Geometry&& g);
    ~Geometry();

    Geometry& operator= (const Geometry& g);
    Geometry& operator= (const Geometry* g);

    void SetVertexData(u32 VertexCount, u32 VertexBufferOffset);
    void SetIndexData(u32 IndexCount, u32 IndexBufferOffset);

    void Destroy();
};