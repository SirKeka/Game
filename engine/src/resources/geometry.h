#pragma once
#include "containers/mstring.hpp"
#include "systems/material_system.h"
#include "math/extents.h"
#include "math/vertex.h"

constexpr int GEOMETRY_NAME_MAX_LENGTH = 256;

// Максимальное количество одновременно загружаемых геометрий
// ЗАДАЧА: сделать настраиваемым
constexpr int VULKAN_MAX_GEOMETRY_COUNT = 4096;

struct Geometry {
    u32 id;                // Идентификатор геометрии
    u32 InternalID;        // Внутренний идентификатор геометрии, используемый растеризатором для сопоставления с внутренними ресурсами.
    u16 generation;        // Генерация геометрии. Увеличивается каждый раз при изменении геометрии.
    FVec3 center;          // Центр геометрии в локальных координатах.
    Extents3D extents;     // Протяженность геометрии в локальных координатах.

    u32 VertexCount;       // Количество вершин данной герметрии
    u32 VertexElementSize; // Размер каждой вершины.
    void* vertices;        // Данные вершин

    u32 IndexCount;        // Количество вершин в данной геометрии
    u32 IndexElementSize;  // Размер одной вершины
    void* indices;         // Данные вершин

    char name[GEOMETRY_NAME_MAX_LENGTH];
    struct Material* material;

    constexpr Geometry() : id(), InternalID(INVALID::ID), generation(INVALID::U16ID), name(), material(nullptr) {}
    // Geometry(u32 id, u16 generation) : id(id), InternalID(INVALID::ID), generation(generation), name(), material(nullptr) {}
    constexpr Geometry(const char* name) : id(), InternalID(INVALID::ID), generation(INVALID::U16ID), material(nullptr) {MemorySystem::CopyMem(this->name, name, GEOMETRY_NAME_MAX_LENGTH);}
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

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Unknown); }
    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Unknown); }
};
