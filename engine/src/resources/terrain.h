#pragma once

#include "core/frame_data.hpp"
#include "defines.hpp"
#include "math/vector2d_fwd.hpp"
#include "math/vector3d_fwd.hpp"
#include "math/vector4d_fwd.hpp"
#include "math/transform.hpp"
#include "geometry.hpp"

/*
 * Необходимо изменить структуру/функции геометрии, чтобы разрешить использование нескольких материалов.
 * Напишите шейдер для обработки веса/смешивания материалов.
 * Использование нескольких материалов (объединение карт с использованием UV-смещений)?
 * Загрузить конфигурацию ландшафта из файла
 * Load heightmaps calculate normals/tangents (copy-pasta of existing, but using terrain_vertex structure) New material type? (terrain/multi material)
*/

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

struct TerrainConfig {
    MString name;
    u32 TileCountX;
    u32 TileCountZ;
    f32 TileScaleX; // Насколько велика каждая плитка по оси x.
    f32 TileScaleZ; // Насколько велика каждая плитка по оси z.
    f32 ScaleY;     // Максимальная высота сгенерированной местности.

    Transform xform;

    u32 VertexDataLength;
    TerrainVertexData *VertexDatas;

    u32 MaterialCount;
    char **MaterialNames;
};

struct Terrain {
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

    Geometry geo;

    u32 MaterialCount;
    char **MaterialNames;

    bool Create(const TextureConfig& config);
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    bool Update();
};


