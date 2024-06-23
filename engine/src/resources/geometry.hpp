#pragma once
#include "containers/mstring.hpp"
#include "math/vertex.hpp"
#include "systems/material_system.hpp"

constexpr int GEOMETRY_NAME_MAX_LENGTH = 256;

// Максимальное количество одновременно загружаемых геометрий
// СДЕЛАТЬ: сделать настраиваемым
constexpr int VULKAN_MAX_GEOMETRY_COUNT = 4096;

struct GeometryID {
    u32 id;
    u32 InternalID;
    u32 generation;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    class Material* material;
    GeometryID(u32 id, u32 generation) : id(id), InternalID(INVALID::ID), generation(generation), name(), material(nullptr) {}
    GeometryID(const char* name) : id(INVALID::ID), InternalID(INVALID::ID), generation(INVALID::ID), material(nullptr) {MMemory::CopyMem(this->name, name, GEOMETRY_NAME_MAX_LENGTH);}
};

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
class Geometry {
private:
    u32 id;
    u32 generation;
    //char name[GEOMETRY_NAME_MAX_LENGTH];

    u32 VertexCount;
    u32 VertexElementSize;
    u64 VertexBufferOffset;
    u32 IndexCount;
    u32 IndexElementSize;
    u64 IndexBufferOffset;

    //friend class GeometrySystem;
    friend class VulkanAPI;
public:
    constexpr Geometry() : id(INVALID::ID), generation(INVALID::ID), VertexCount(), VertexElementSize(), VertexBufferOffset(), IndexCount(), IndexElementSize(), IndexBufferOffset() {}
    constexpr Geometry(u32 VertexCount, u32 VertexElementSize, u32 IndexCount): id(), generation(), VertexCount(VertexCount), VertexElementSize(VertexElementSize), VertexBufferOffset(), IndexCount(IndexCount), IndexElementSize(sizeof(u32)), IndexBufferOffset() {}
    constexpr Geometry(const Geometry& g) : id(g.id), generation(g.generation), VertexCount(g.VertexCount), VertexElementSize(g.VertexElementSize), VertexBufferOffset(g.VertexBufferOffset), IndexCount(g.IndexCount), IndexElementSize(g.IndexElementSize), IndexBufferOffset(g.IndexBufferOffset) {}
    constexpr Geometry(Geometry&& g);
    ~Geometry();

    Geometry& operator= (const Geometry& g);
    Geometry& operator= (const Geometry* g);

    void SetVertexData(u32 VertexCount, u32 ElementSize, u64 VertexBufferOffset);
    void SetIndexData(u32 IndexCount, u64 IndexBufferOffset);

    void Destroy();
};