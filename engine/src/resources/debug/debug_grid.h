#pragma once
#include "defines.hpp"
#include "math/extents.h"
#include "math/vertex.h"
#include "resources/geometry.h"
#include "core/identifier.h"

struct DebugGrid {
    enum Orientation {
        /// @brief Сетка, которая лежит «плоско» в мире вдоль плоскости земли (y-плоскости). Это конфигурация по умолчанию.
        XZ = 0,
        /// @brief Сетка, которая лежит на z-плоскости (по умолчанию обращена к экрану, ортогонально плоскости земли).
        XY = 1,
        /// @brief Сетка, которая лежит на x-плоскости (ортогонально плоскости экрана по умолчанию и плоскости земли).v
        YZ = 2,
    };

    struct Config {
        MString name;
        Orientation orientation;
        /// @brief Количество пространства в первом измерении ориентации с обоих направлений наружу от начала координат.
        u32 TileCountDim0;
        /// @brief Количество пространства во втором измерении ориентации с обоих направлений наружу от начала координат.
        u32 TileCountDim1;
        /// @brief Насколько велика каждая плитка на обеих осях относительно одной единицы (по умолчанию = 1,0).
        f32 TileScale;

        /// @brief Указывает, должна ли быть отрисована третья ось.
        bool UseThirdAxis;

        /// @brief Общее количество вершин.
        u32 VertexLength;
        /// @brief Сгенерированные данные вершин.
        ColourVertex3D* vertices;
    };
    

    u32 UniqueID;
    MString name;
    Orientation orientation;
    /// @brief Количество пространства в первом измерении ориентации с обоих направлений наружу от начала координат.
    u32 TileCountDim0;
    /// @brief Количество пространства во втором измерении ориентации с обоих направлений наружу от начала координат.
    u32 TileCountDim1;
    /// @brief Насколько велика каждая плитка на обеих осях относительно одной единицы (по умолчанию = 1,0).
    f32 TileScale;

    /// @brief Указывает, должна ли быть отрисована третья ось.
    bool UseThirdAxis;

    Extents3D extents;
    FVec3 origin;

    u32 VertexCount;
    ColourVertex3D* vertices;

    GeometryID geometry;

    constexpr DebugGrid() : UniqueID(), name(), orientation(), TileCountDim0(), TileCountDim1(), TileScale(), UseThirdAxis(), extents(), origin(), VertexCount(), vertices(nullptr) {}
    constexpr DebugGrid(Config& config)
    :
    UniqueID(Identifier::AquireNewID(this)),
    name((MString &&)config.name),
    orientation(config.orientation),
    TileCountDim0(config.TileCountDim0),
    TileCountDim1(config.TileCountDim1),
    TileScale(config.TileScale),
    UseThirdAxis(config.UseThirdAxis),
    extents(),
    origin(),
    // 2 вершины на линию, 1 линия на плитку в каждом направлении, плюс одна в середине для каждого направления. Добавляем еще 2 для третьей оси.
    VertexCount(((TileCountDim0 * 2 + 1) * 2) + ((TileCountDim1 * 2 + 1) * 2) + 2),
    vertices(nullptr),
    geometry()
    {
        f32 Max0 = TileCountDim0 * TileScale;
        f32 Min0 = -Max0;
        f32 Max1 = TileCountDim1 * TileScale;
        f32 Min1 = -Max1;
    
        switch (orientation) {
            default:
            case Orientation::XZ:
                extents.min.x = Min0;
                extents.max.x = Max0;
                extents.min.z = Min1;
                extents.max.z = Max1;
                break;
            case Orientation::XY:
                extents.min.x = Min0;
                extents.max.x = Max0;
                extents.min.y = Min1;
                extents.max.y = Max1;
                break;
            case Orientation::YZ:
                extents.min.y = Min0;
                extents.max.y = Max0;
                extents.min.z = Min1;
                extents.max.z = Max1;
                break;
        }
    }
    // ~DebugGrid();
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    bool Update();
};