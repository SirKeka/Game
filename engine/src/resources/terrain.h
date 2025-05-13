#pragma once

#include "core/frame_data.h"
#include "defines.hpp"
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
 * Load heightmaps calculate normals/tangents (copy-pasta of existing, but using terrain_vertex structure) New material type? (terrain/multi material)
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
        u32 TileCountX;
        u32 TileCountZ;
        f32 TileScaleX; // Насколько велика каждая плитка по оси x.
        f32 TileScaleZ; // Насколько велика каждая плитка по оси z.
        f32 ScaleY;     // Максимальная высота сгенерированной местности.

        Transform xform;

        // u32 VertexDataLength;
        DArray<TerrainVertexData> VertexDatas;

        u32 MaterialCount;
        char** MaterialNames;
    };

    u32 UniqueID;
    MString name;
    Transform xform;
    u32 TileCountX;
    u32 TileCountZ;
    f32 TileScaleX; // Насколько велика каждая плитка по оси x.
    f32 TileScaleZ; // Насколько велика каждая плитка по оси z.
    f32 ScaleY;     // Максимальная высота сгенерированной местности.

    u32 VertexDataLength;
    TerrainVertexData* VertexDatas;

    Extents3D extents;
    FVec3 origin;

    u32 VertexCount;
    TerrainVertex* vertices;

    u32 IndexCount;
    u32* indices;

    GeometryID geo;

    u32 MaterialCount;
    char** MaterialNames; // DArray<MString>?

    Terrain() : name(), xform(), TileCountX(), TileCountZ(), TileScaleX(), TileScaleZ(), ScaleY(), VertexDatas(), extents(), origin(), VertexCount(), vertices(nullptr), IndexCount(), indices(nullptr), geo(), MaterialCount(), MaterialNames(nullptr) {}

    bool Create(Config& config);
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    bool Update();
};


