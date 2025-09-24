#pragma once

#include "vector2d_fwd.h"

struct Circle
{
    FVec2 center; // Центральная точка кольца
    f32 raadius;  // Радиус кольца
};

namespace Math
{
    MINLINE bool PointInCircle(const FVec2& point, const Circle& circle) {
        f32 rSquared = circle.radius * circle.radius;
        return DistanceSquared(point, circle.center) <= rSquared;
    }
} // namespace Math

    