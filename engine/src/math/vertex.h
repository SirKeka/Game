#pragma once
#include "vector2d_fwd.h"
#include "vector3d_fwd.h"
#include "vector4d_fwd.h"
#include "core/memory_system.h"

struct Vertex2D
{
    FVec2 position; // Позиция вершины
    FVec2 texcoord; // Текстурные коорднаты

    constexpr Vertex2D() : position(), texcoord() {}
    constexpr Vertex2D(FVec2 position, FVec2 texcoord) : position(position), texcoord(texcoord) {}
};

/// @brief Структура вершины геометрии/сетки(меша) содержит:
/// FVec3 position - Позиция вершины
/// FVec3 normal   - Нормаль вершины
/// FVec2 texcoord - Текстурные коорднаты
/// FVec4 colour   - Цвет вершины
/// FVec3 tangent  - Касательная вершины.
struct Vertex3D
{
    FVec3 position; // Позиция вершины
    FVec3 normal;   // Нормаль вершины
    FVec2 texcoord; // Текстурные коорднаты
    FVec4 colour;   // Цвет вершины
    FVec3 tangent;  // Касательная вершины.

    constexpr Vertex3D() : position(), normal(), texcoord(), colour(), tangent() {}

    bool operator==(const Vertex3D& v3d) const {
        if (position == v3d.position && normal == v3d.normal && texcoord == v3d.texcoord && colour == v3d.colour && tangent == v3d.tangent) {
            return true;
        }
        return false;
    }
};

/// @brief Представляет одну вершину в трехмерном пространстве только с данными о положении и цвете.
struct ColourVertex3D
{
    /// @brief Положение вершины. w игнорируется.
    FVec4 position;
    /// @brief Цвет вершины.
    FVec4 colour;
};
