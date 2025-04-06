#pragma once
#include "vector2d_fwd.h"
#include "vector3d_fwd.h"

///@brief Представляет размеры 2D-объекта.
struct Extents2D {
    FVec2 min;  // Минимальные размеры объекта.
    FVec2 max;  // Максимальные размеры объекта.
    // constexpr Extents2D() : min(), max() {}
    // constexpr Extents2D(FVec2 FVmin, FVec2 FVmax) : min(min), max(max) {}
};

/// @brief Представляет размеры 3D-объекта.
struct Extents3D {
    FVec3 min;  // Минимальные размеры объекта.
    FVec3 max;  // Максимальные размеры объекта.
    // constexpr Extents3D() : min(), max() {}
    // constexpr Extents3D(FVec3 min, FVec3 max) : min(min), max(max) {}
};