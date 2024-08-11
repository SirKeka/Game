#pragma once
#include "vector2d_fwd.hpp"
#include "vector3d_fwd.hpp"

///@brief Представляет размеры 2D-объекта.
struct Extents2D {
    FVec2 MinSize;      // Минимальные размеры объекта.
    FVec2 MaxSize;      // Максимальные размеры объекта.
    constexpr Extents2D() : MinSize(), MaxSize() {}
    constexpr Extents2D(FVec2 MinSize, FVec2 MaxSize) : MinSize(MinSize), MaxSize(MaxSize) {}
};

/// @brief Представляет размеры 3D-объекта.
struct Extents3D {
    FVec3 MinSize;      // Минимальные размеры объекта.
    FVec3 MaxSize;      // Максимальные размеры объекта.
    constexpr Extents3D() : MinSize(), MaxSize() {}
    constexpr Extents3D(FVec3 MinSize, FVec3 MaxSize) : MinSize(MinSize), MaxSize(MaxSize) {}
};