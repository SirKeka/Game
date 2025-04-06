#pragma once
#include "containers/mstring.hpp"
#include "systems/material_system.h"
#include "math/extents.h"
#include "math/vertex.h"

constexpr int GEOMETRY_NAME_MAX_LENGTH = 256;

// Максимальное количество одновременно загружаемых геометрий
// ЗАДАЧА: сделать настраиваемым
constexpr int VULKAN_MAX_GEOMETRY_COUNT = 4096;

struct GeometryID {
    u32 id;
    u32 InternalID;
    u16 generation;
    FVec3 center;
    Extents3D extents;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    struct Material* material;

    constexpr GeometryID() : id(INVALID::ID), InternalID(INVALID::U16ID), generation(INVALID::U16ID), name(), material(nullptr) {}
    // GeometryID(u32 id, u16 generation) : id(id), InternalID(INVALID::ID), generation(generation), name(), material(nullptr) {}
    constexpr GeometryID(const char* name) : id(INVALID::ID), InternalID(INVALID::ID), generation(INVALID::U16ID), material(nullptr) {MemorySystem::CopyMem(this->name, name, GEOMETRY_NAME_MAX_LENGTH);}
    void* operator new[](u64 size) { return MemorySystem::Allocate(size, Memory::Array); }
    void operator delete[](void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Array); }
};

/// @brief Представляет конфигурацию геометрии.
/// @param u32_VertexSize размер каждой вершины.
/// @param u32_VertexCount количество вершин.
/// @param void*_vertices массив вершин.
/// @param u32_IndexSize размер каждого индекса.
/// @param u32_IndexCount количество индексов.
/// @param void*_indices массив индексов.
/// @param char_name[] имя геометрии.
/// @param char_MaterialName[] имя материала, используемого геометрией.
/// @param FVec3_center 
/// @param FVec3_MinExtents 
/// @param FVec3_MaxExtents 
struct MAPI GeometryConfig {
    u32 VertexSize;
    u32 VertexCount;
    void* vertices;
    u32 IndexSize;
    u32 IndexCount;
    u32* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char MaterialName[MATERIAL_NAME_MAX_LENGTH];

    FVec3 center{};
    FVec3 MinExtents{};
    FVec3 MaxExtents{};

    /// @brief Освобождает ресурсы, имеющиеся в указанной конфигурации.
    /// @param config ссылка на конфигурацию, которую нужно удалить.
    void Dispose();

    /// @brief Копирует имя и название материала.
    /// @param name имя геометрии.
    /// @param MaterialName имя материала, который используется геометрией.
    void CopyNames(const char *name, const char *MaterialName);

    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
class MAPI Geometry {
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
    friend struct Terrain;
public:
    constexpr Geometry() : id(INVALID::ID), generation(), VertexCount(), VertexElementSize(), VertexBufferOffset(), IndexCount(), IndexElementSize(), IndexBufferOffset() {}
    constexpr Geometry(u32 VertexCount, u32 VertexElementSize, u32 IndexCount): id(), generation(), VertexCount(VertexCount), VertexElementSize(VertexElementSize), VertexBufferOffset(), IndexCount(IndexCount), IndexElementSize(sizeof(u32)), IndexBufferOffset() {}
    constexpr Geometry(const Geometry& g) : id(g.id), generation(g.generation), VertexCount(g.VertexCount), VertexElementSize(g.VertexElementSize), VertexBufferOffset(g.VertexBufferOffset), IndexCount(g.IndexCount), IndexElementSize(g.IndexElementSize), IndexBufferOffset(g.IndexBufferOffset) {}
    constexpr Geometry(Geometry&& g);
    ~Geometry();

    Geometry& operator= (const Geometry& g);
    Geometry& operator= (const Geometry* g);

    void SetVertexIndex(u32 VertexCount, u32 VertexElementSize, u32 IndexCount);
    void SetVertexData(u32 VertexCount, u32 ElementSize, u64 VertexBufferOffset);
    void SetIndexData(u32 IndexCount, u64 IndexBufferOffset);

    void Destroy();

};