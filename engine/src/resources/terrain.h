#pragma once

#include "core/frame_data.h"
#include "core/identifier.h"
#include "math/vector2d_fwd.h"
#include "math/vector3d_fwd.h"
#include "math/vector4d_fwd.h"
#include "math/transform.h"
#include "geometry.h"

/*
 * Необходимо изменить структуру/функции геометрии, чтобы разрешить использование нескольких материалов.
 * Напишите шейдер для обработки веса/смешивания материалов.
 * Использование нескольких материалов (объединение карт с использованием UV-смещений)?
 * Загрузить конфигурацию ландшафта из файла
 * Загрузка карт высот, расчет нормалей/касательных (копирование-вставка существующих, но с использованием структуры terrain_vertex) Новый тип материала? (рельеф/мультиматериал)
*/

constexpr int TERRAIN_MAX_MATERIAL_COUNT = 4;
struct TerrainVertex {
    /** @brief Положение вершины */
    FVec3 position;
    /** @brief Нормаль вершины. */
    FVec3 normal;
    /** @brief Текстурная координата вершины. */
    FVec2 texcoord;
    /** @brief Цвет вершины. */
    FVec4 colour;
    /** @brief Касательная вершины. */
    FVec4 tangent;

    /** @brief Набор весов материалов для этой вершины. */
    f32 MaterialWeights[TERRAIN_MAX_MATERIAL_COUNT];
};

struct TerrainVertexData {
    f32 height;
};

struct Terrain {
    struct Config {
        MString name;
        u32 ChunkSize;
        u32 TileCountX;
        u32 TileCountZ;
        f32 TileScaleX; // Насколько велика каждая плитка по оси x.
        f32 TileScaleZ; // Насколько велика каждая плитка по оси z.
        f32 ScaleY;     // Максимальная высота сгенерированной местности.

        Transform xform;

        u32 VertexDataLength;
        TerrainVertexData* VertexDatas;

        u32 MaterialCount;
        MString* MaterialNames;
    };

    struct ChunkLOD {
        u32 SurfaceIndexCount; // Количество индексов для поверхности фрагмента.

        u32 TotalIndexCount;   // Общее количество индексов, включая индексы для боковых юбок.
        u32 *indices;          // Данные индекса.
        u64 IndexBufferOffset; // Смещение от начала буфера индексов.
    };

    struct Chunk {
        u16 generation; // Версия генерации чанка. Увеличивается при каждом изменении геометрии.
        u32 SurfaceVertexCount;
        u32 TotalVertexCount;
        TerrainVertex *vertices;
        u64 VertexBufferOffset;

        ChunkLOD *lods;

        FVec3 center;      // Центр геометрии в локальных координатах.
        Extents3D extents; // Габариты геометрии в локальных координатах.

        struct Material *material; // Указатель на материал, связанный с этой геометрией.

        u8 CurrentLOD;
    };

    Identifier id;
    u32 generation;
    MString name;
    Transform xform;
    u32 TileCountX;
    u32 TileCountZ;
    f32 TileScaleX; // Насколько велика каждая плитка по оси x.
    f32 TileScaleZ; // Насколько велика каждая плитка по оси z.
    f32 ScaleY;     // Максимальная высота сгенерированной местности.

    u32 ChunkSize;

    u32 VertexDataLength;
    TerrainVertexData* VertexDatas{};

    u32 TotalVertexCount;
    u32 TotalIndexCount;
    TerrainVertex* vertices{};
    u32* indices{};

    Extents3D extents;
    FVec3 origin;

    u32 ChunkCount;
    // построчно, затем по столбцам
    // 0, 1, 2, 3
    // 4, 5, 6, 7
    // 8, 9, ...
    Chunk *chunks;

    u8 LODcount;

    u32 MaterialCount;
    MString* MaterialNames; // DArray<MString>?
    
    constexpr Terrain() : id(), generation(INVALID::ID), name(), xform(), TileCountX(), TileCountZ(), TileScaleX(), TileScaleZ(), ScaleY(), ChunkSize(),
    VertexDataLength(), VertexDatas(), extents(), origin(), ChunkCount(), chunks(), LODcount(), MaterialCount(), MaterialNames(nullptr) {}

    MAPI bool Create(Config& config);
    MAPI void Destroy();

    MAPI bool Initialize();
    MAPI bool Load();
    MAPI bool ChunkLoad(Chunk& chunk);
    MAPI bool Unload();
    MAPI bool ChunkUnload(Chunk& chunk);

    MAPI bool Update();
private:
    void Destroy(Chunk& chunk);
    // Вычисляет данные вершин, а также подготавливает данные индексов для каждого уровня детализации (LOD) данного чанка.
    void CalculateGeometry(Chunk& chunk, u32 ChunkOffsetX, u32 ChunkOffsetZ);
};


