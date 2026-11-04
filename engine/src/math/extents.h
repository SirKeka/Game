#pragma once
#include "vector2d_fwd.h"
#include "vector3d_fwd.h"

///@brief Представляет размеры 2D-объекта.
struct Extents2D 
{
    FVec2 min;  // Минимальные размеры объекта.
    FVec2 max;  // Максимальные размеры объекта.

    MINLINE FVec2 Half() {
        return FVec2(
            (min.x + max.x) * 0.5F,
            (min.y + max.y) * 0.5F
        );
    }
};

/// @brief Представляет размеры 3D-объекта.
struct Extents3D 
{
    FVec3 min;  // Минимальные размеры объекта.
    FVec3 max;  // Максимальные размеры объекта.

    MINLINE FVec3 Half() {
        return FVec3(
            (min.x + max.x) * 0.5F,
            (min.y + max.y) * 0.5F,
            (min.z + max.z) * 0.5F
        );
    }
};



