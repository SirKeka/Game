#pragma once
#include "containers/mstring.hpp"
#include "math/vertex.hpp"
#include "systems/material_system.hpp"
#include "math/extents.hpp"

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
    class Material* material;
    GeometryID(u32 id, u16 generation) : id(id), InternalID(INVALID::ID), generation(generation), name(), material(nullptr) {}
    GeometryID(const char* name) : id(INVALID::ID), InternalID(INVALID::ID), generation(INVALID::U16ID), material(nullptr) {MMemory::CopyMem(this->name, name, GEOMETRY_NAME_MAX_LENGTH);}
    void* operator new[](u64 size) { return MMemory::Allocate(size, Memory::Array); }
    void operator delete[](void* ptr, u64 size) { MMemory::Free(ptr, size, Memory::Array); }
};

struct GeometryConfig {
    u32 VertexSize;
    u32 VertexCount;
    void* vertices;
    u32 IndexSize;
    u32 IndexCount;
    void* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char MaterialName[MATERIAL_NAME_MAX_LENGTH];

    FVec3 center;
    FVec3 MinExtents;
    FVec3 MaxExtents;

    constexpr GeometryConfig() 
    : VertexSize(), VertexCount(), vertices(nullptr), IndexSize(), IndexCount(), indices(nullptr), name(), MaterialName(), center(), MinExtents(), MaxExtents() {}
    constexpr GeometryConfig(u32 VertexSize, u32 VertexCount, void* vertices, u32 IndexSize, u32 IndexCount, void* indices, const char* name, const char* MaterialName)
    : VertexSize(VertexSize), 
    VertexCount(VertexCount), 
    vertices(vertices), 
    IndexSize(IndexSize), 
    IndexCount(IndexCount), 
    indices(indices), 
    name(), 
    MaterialName() {
        for (u64 i = 0, j = 0; i < 256;) {
            if(name[i]) {
                this->name[i] = name[i];
                i++;
            }
            if(MaterialName[j]) {
                this->MaterialName[j] = MaterialName[j];
                j++;
            }
            if (!name[i] && !MaterialName[j]) {
                break;
            }
        }
    }
    constexpr GeometryConfig(u32 VertexSize, u32 VertexCount, void* vertices, u32 IndexSize, u32 IndexCount, void* indices, const char* name, const char* MaterialName, FVec3 center, FVec3 MinExtents, FVec3 MaxExtents)
    : 
    VertexSize(VertexSize), 
    VertexCount(VertexCount), 
    vertices(vertices), 
    IndexSize(IndexSize), 
    IndexCount(IndexCount), 
    indices(indices), 
    name(), 
    MaterialName(), 
    center(center), 
    MinExtents(MinExtents), 
    MaxExtents(MaxExtents)  {
        for (u64 i = 0, j = 0; i < 256;) {
            if(name[i]) {
                this->name[i] = name[i];
                i++;
            }
            if(MaterialName[j]) {
                this->MaterialName[j] = MaterialName[j];
                j++;
            }
            if (!name[i] && !MaterialName[j]) {
                break;
            }
        }
    }
    constexpr GeometryConfig(const GeometryConfig& conf)
    : VertexSize(conf.VertexSize), VertexCount(conf.VertexCount), vertices(MMemory::Allocate(VertexCount * VertexSize, Memory::Array)), IndexSize(conf.IndexSize), IndexCount(conf.IndexCount), indices(MMemory::Allocate(IndexCount * IndexSize, Memory::Array)), name(), MaterialName(), center(conf.center), MinExtents(conf.MinExtents), MaxExtents(conf.MaxExtents) {
        MMemory::CopyMem(vertices, conf.vertices, VertexCount * VertexSize);
        MMemory::CopyMem(indices, conf.indices, IndexCount * IndexSize);
        CopyNames(conf.name, conf.MaterialName);
    }
    constexpr GeometryConfig(GeometryConfig&& conf) : VertexSize(conf.VertexSize), VertexCount(conf.VertexCount), vertices(conf.vertices), IndexSize(conf.IndexSize), IndexCount(conf.IndexCount), indices(conf.indices), name(), MaterialName(), center(conf.center), MinExtents(conf.MinExtents), MaxExtents(conf.MaxExtents) {
        CopyNames(conf.name, conf.MaterialName);
        conf.VertexSize = 0;
        conf.VertexCount = 0;
        conf.vertices = nullptr;
        conf.IndexSize = 0;
        conf.IndexCount = 0;
        conf.indices = nullptr;
        conf.center = FVec3();
        conf.MinExtents = FVec3();
        conf.MaxExtents = FVec3();
    }
    GeometryConfig& operator =(const GeometryConfig& conf) {
        VertexSize = conf.VertexSize;
        VertexCount = conf.VertexCount;
        vertices = MMemory::Allocate(VertexCount * VertexSize, Memory::Array);
        MMemory::CopyMem(vertices, conf.vertices, VertexCount * VertexSize);
        IndexSize = conf.IndexSize;
        IndexCount = conf.IndexCount;
        indices = MMemory::Allocate(IndexCount * IndexSize, Memory::Array);
        MMemory::CopyMem(indices, conf.indices, IndexCount * IndexSize);
        CopyNames(conf.name, conf.MaterialName);
        center = conf.center;
        MinExtents = conf.MinExtents;
        MaxExtents = conf.MaxExtents;
        return *this;
    }
    GeometryConfig& operator =(GeometryConfig&& conf) {
        VertexSize = conf.VertexSize;
        VertexCount = conf.VertexCount;
        vertices = conf.vertices;
        IndexSize = conf.IndexSize;
        IndexCount = conf.IndexCount;
        indices = conf.indices;
        CopyNames(conf.name, conf.MaterialName);
        center = conf.center;
        MinExtents = conf.MinExtents;
        MaxExtents = conf.MaxExtents;

        conf.VertexSize = 0;
        conf.VertexCount = 0;
        conf.vertices = nullptr;
        conf.IndexSize = 0;
        conf.IndexCount = 0;
        conf.indices = nullptr;
        conf.center = FVec3();
        conf.MinExtents = FVec3();
        conf.MaxExtents = FVec3();

        return *this;
    }

    /// @brief Освобождает ресурсы, имеющиеся в указанной конфигурации.
    /// @param config ссылка на конфигурацию, которую нужно удалить.
    void Dispose();
private:
    /// @brief 
    /// @param name 
    /// @param MaterialName 
    void CopyNames(const char (&name)[256], const char (&MaterialName)[256]);
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

    void SetVertexIndex(u32 VertexCount, u32 VertexElementSize, u32 IndexCount);
    void SetVertexData(u32 VertexCount, u32 ElementSize, u64 VertexBufferOffset);
    void SetIndexData(u32 IndexCount, u64 IndexBufferOffset);

    void Destroy();

};