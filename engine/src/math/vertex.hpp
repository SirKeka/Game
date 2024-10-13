#pragma once
#include "vector2d_fwd.hpp"
#include "vector3d_fwd.hpp"
#include "vector4d_fwd.hpp"
#include "core/mmemory.hpp"

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

    const bool operator==(const Vertex3D& v3d) {
        if (position == v3d.position && normal == v3d.normal && texcoord == v3d.texcoord && colour == v3d.colour && tangent == v3d.tangent) {
            return true;
        }
        return false;
    }
};

